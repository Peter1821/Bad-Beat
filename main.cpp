// Bad Beat -- entry point.
//
// Deliberately thin: set up the terminal, start a run, hand control to GameApp.
// Everything about how the game looks lives in src/ui/, and everything about how
// it works lives in badbeat_core.

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "core/Rng.h"
#include "core/Vec2.h"
#include "ui/GameApp.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// <wingdi.h> defines RGB(r,g,b) as a macro, which silently mangles any call to
// ftxui::Color::RGB(...) into a syntax error. We want the FTXUI one.
#undef RGB
#endif

namespace {

/// Replays a string of keys against the app.
///
/// Lets --dump-ui drive the game without a terminal or a human watching, which
/// is the only way to check an interactive screen in an automated run.
void ApplyKeys(bb::ui::GameApp& app, const std::string& keys) {
    for (const char key : keys) {
        // Advancing and quitting belong to the app, not to any one screen.
        if (key == 'n') {
            app.Advance();
            continue;
        }
        if (key == 'Q') {
            app.RequestQuit();
            continue;
        }
        if (key == 'C') {
            app.CancelQuit();
            continue;
        }

        // Away from combat, the arrow keys move a list selection rather than a
        // board cursor.
        if (bb::ui::MapScreen* map = app.ActiveMap()) {
            if (key == 'l' || key == 'j') map->SelectNext();
            if (key == 'h' || key == 'k') map->SelectPrevious();
            continue;
        }
        if (bb::ui::RewardScreen* reward = app.ActiveReward()) {
            if (key == 'l' || key == 'j') reward->SelectNext();
            if (key == 'h' || key == 'k') reward->SelectPrevious();
            continue;
        }
        if (bb::ui::ShopScreen* shop = app.ActiveShop()) {
            if (key == 'l' || key == 'j') shop->SelectNext();
            if (key == 'h' || key == 'k') shop->SelectPrevious();
            continue;
        }
        if (bb::ui::EventScreen* text_event = app.ActiveEvent()) {
            if (key == 'l' || key == 'j') text_event->SelectNext();
            if (key == 'h' || key == 'k') text_event->SelectPrevious();
            continue;
        }

        bb::ui::CombatScreen* combat = app.ActiveCombat();
        if (combat == nullptr) continue;

        switch (key) {
            case 'h': combat->MoveCursor(bb::kWest); break;
            case 'j': combat->MoveCursor(bb::kSouth); break;
            case 'k': combat->MoveCursor(bb::kNorth); break;
            case 'l': combat->MoveCursor(bb::kEast); break;
            case 'c': combat->SnapCursorToPlayer(); break;
            case 'x': combat->ClearSelection(); break;
            // Lower case does the whole thing in one go; upper case stages the
            // action and stops, so a script can inspect the confirmation prompt.
            case 'e': combat->Confirm(); break;
            case 't': combat->EndTurn(); break;
            case 'E': combat->RequestConfirm(); break;
            case 'T': combat->RequestEndTurn(); break;
            case 'y': combat->CommitPending(); break;
            case 'z': combat->CancelPending(); break;
            default:
                if (key >= '1' && key <= '9') combat->SelectCard(key - '1');
                break;
        }
    }
}

/// Renders one frame to a fixed-size off-screen buffer and prints it.
///
/// Colour is not reproduced, so this verifies structure, alignment, cursor
/// position and threat markers -- never appearance.
void DumpUi(const bb::ui::GameApp& app, int width, int height) {
    using namespace ftxui;

    Element document = app.Render();
    Screen buffer = Screen::Create(Dimension::Fixed(width), Dimension::Fixed(height));
    Render(buffer, document);
    std::cout << buffer.ToString() << '\n';
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    // FTXUI enables virtual-terminal processing itself, but the output code
    // page still has to be UTF-8 or the box-drawing glyphs arrive as mojibake.
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
#endif

    using namespace ftxui;

    // A fixed seed in dump mode so scripted checks always see the same run; a
    // real one otherwise. An optional third argument overrides it.
    const bool dumping = argc > 1 && std::string(argv[1]) == "--dump-ui";
    std::uint64_t seed = 20260731ULL;
    if (!dumping) {
        seed = bb::Rng::FromEntropy().Seed();
    } else if (argc > 3) {
        seed = std::strtoull(argv[3], nullptr, 10);
    }

    bb::ui::GameApp app(seed);

    if (dumping) {
        if (argc > 2) ApplyKeys(app, argv[2]);
        DumpUi(app, 130, 42);
        return 0;
    }

    auto screen = ScreenInteractive::Fullscreen();
    auto root = app.Component();

    // Quit is handled here rather than inside the app so that no screen can
    // kill the process on its own. This wrapper sees events first, so while the
    // prompt is up nothing underneath can act on a keypress meant for it.
    root |= CatchEvent([&](Event event) {
        if (app.QuitPending()) {
            if (event == Event::Return || event == Event::Character('y')) {
                screen.Exit();
                return true;
            }

            app.CancelQuit();
            if (event == Event::Escape || event == Event::Character('n')) return true;

            // Any other key both cancels and acts, so changing your mind costs
            // one keypress rather than two.
            return false;
        }

        if (event == Event::Character('q')) {
            app.RequestQuit();
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}
