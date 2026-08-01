# Bad Beat — Game Design Document

> Working document, for me. Records what's decided, *why* it was decided, and
> where the room to expand is.
>
> Status markers: **[built]** exists in code · **[next]** planned for the slice ·
> **[idea]** deliberately not committed to.

---

## 1. What this game is

A turn-based tactical roguelike where your actions are **cards with a shape on
the grid**. You draw a hand each turn, and where you're standing decides which
of those cards is worth anything.

**The one-line pitch:** Slay the Spire's deck, played on a tactics grid, where
the right card in the wrong position does nothing.

### The name

A *bad beat* is a poker hand that should have won and didn't. Statistically
sound play, ruined by the river. That's the intended feeling: you have the
answer in hand, and the board doesn't let you use it.

### Pillars

1. **Position is the real resource.** Damage numbers are the boring half. The
   interesting half is whether you can be in the tile that makes them apply.
2. **Every threat is visible before it lands.** Enemies telegraph both *what*
   they'll do and *which tiles* it will hit. Losing HP should feel like a
   misplay, never an ambush.
3. **Small, legible numbers.** 30 HP, 3 energy, 8×6 grid. A player should be
   able to hold the whole board state in their head.
4. **Short runs.** 5–8 nodes. A full run in one sitting.

### What separates this from Slay the Spire

In StS you answer a threat by playing the correct card. Here you can also answer it by
**standing somewhere else**. A defence card and a movement card are competing
solutions to the same problem, and that trade is the game.

If a fight can be won without ever moving, that fight is badly designed, in my opinion.

---

## 2. Core loop

```
        ┌──────────────────────────────────────────┐
        │                                          │
   ┌────▼─────┐   ┌──────────┐   ┌────────┐   ┌────┴─────┐
   │ Map node │──▶│  Combat  │──▶│ Reward │──▶│  Choose  │
   │  choice  │   │          │   │        │   │ next node│
   └──────────┘   └────┬─────┘   └────────┘   └──────────┘
                       │ death
                  ┌────▼─────┐   ┌──────────────┐
                  │ Run over │──▶│ Meta unlocks │──▶ new run
                  └──────────┘   └──────────────┘
```

Permadeath. Deck and HP persist across nodes within a run, and are wiped between
runs. Only meta-progression carries over.

---

## 3. Combat

### 3.1 The battlefield **[built]**

8 wide × 6 tall. Orthogonal movement only — **diagonals are not adjacent**.
That makes Manhattan distance the metric for movement and card range, so a
"range 2" card reaches a diamond, not a square. Square/blast shapes explicitly
opt into Chebyshev distance instead.

Terrain, fixed for the whole fight:

| Tile | Behaviour |
|---|---|
| Floor | Walkable |
| Wall | Blocks movement; will block line of sight once ranged cards need it |
| Hazard | **Walkable**, but damages whatever stands in it |

Hazards being walkable is deliberate: walking into fire should be a bad decision
the player is allowed to make, not an illegal move. Taking that choice away
removes a lever — sometimes eating 3 damage to reach the right tile is correct.

> **[idea]** Destructible walls. **[idea]** Terrain a card can create — a wall
> card that reshapes the board mid-fight would be a strong signature effect.

### 3.2 Turn structure **[built]**

Player phase, then enemy phase. No interleaved initiative — it's much harder to
telegraph readably, and readability is pillar 2.

**Start of player turn:** block clears · energy → 3 · free step resets · draw to 5.

**During:** play cards, move, in any order.

**End of player turn:** discard the whole hand.

**Enemy phase:** each enemy executes the intent it telegraphed *last* turn, then
chooses and displays its next one.

The one-turn delay is the whole tactical engine. An enemy that decided its
attack last turn can be dodged this turn.

> Block clearing at the *start* of your turn (not the end) means block spent on
> defence protects you through the enemy phase, which is the only timing that
> makes defensive cards feel worth a card slot.

### 3.3 Energy and movement **[built]**

3 energy per turn. Unspent energy is lost, never banked — every turn is a
self-contained puzzle rather than a savings account.

**Movement:** one free orthogonal step per turn, plus whatever movement cards
provide. The free step guarantees you're never *completely* stuck behind a bad
draw, while keeping real repositioning a deck-construction decision.

> Tuning knob: if the free step turns out to trivialise dodging, the fix is to
> make more enemy intents cover 2+ tiles rather than to remove the free step.

### 3.4 Cards **[built]**

A card is **data**, not a class. No `Card::Execute()` hierarchy. Each card is:

```
CardDef {
    id, name, cost, rarity
    TargetPattern pattern      // where it can be aimed and what it hits
    vector<EffectOp> effects   // what happens, in order
}
```

This buys three things: cards load from JSON, descriptions can be generated from
the effect list rather than written twice, and an "upgraded" card is just the
same definition with a bumped number.

**Targeting is two stages** — this is the most important system in the game:

```
TargetPattern {
    OriginRule origin   // where you may aim
    int        range    // how far
    Shape      shape    // what gets hit around the aim point
    int        shapeParam
}
```

| Origin rule | Meaning |
|---|---|
| `Self` | Always the caster's tile |
| `AnyTileInRange` | Any tile within range |
| `OccupiedTileInRange` | Must aim at an actor |
| `AdjacentTile` | Orthogonally adjacent only |
| `Direction` | Pick one of 4 directions, not a tile |

| Shape | Meaning |
|---|---|
| `Single` | Just the aimed tile |
| `Line(n)` | n tiles outward from caster through the aim direction |
| `Cone(n)` | Widening wedge |
| `Blast(r)` | Square, Chebyshev radius r |
| `Diamond(r)` | Manhattan radius r |
| `Ring(r)` | The perimeter only — hits around a centre, not the centre |
| `Row` / `Column` | Entire row or column |

One function does the work:

```cpp
std::vector<Vec2> ComputeAffectedTiles(pattern, casterPos, aimPos, battlefield);
```

The UI calls it every frame while the cursor moves, to paint the highlight. The
resolver calls **the same function** when the card is confirmed. One source of
truth, so the preview can never lie about the outcome. If those ever diverge,
the game is broken in the way players hate most.

**Effect vocabulary** (the design space to draw from):

| Effect | Notes |
|---|---|
| `Damage(n)` | Block absorbs first |
| `Block(n)` | Expires at start of your next turn |
| `MoveSelf` | To the aimed tile |
| `Dash(n)` | Up to n tiles in a direction, stopping at obstacles |
| `Push(n)` / `Pull(n)` | Displace a target — **collision damage?** [idea] |
| `Swap` | Trade places with the target |
| `Draw(n)` / `GainEnergy(n)` | Tempo |
| `ApplyStatus(kind, stacks)` | See below |
| `SpawnHazard` | Create terrain |

> **[idea]** Cards whose effect *depends on position* — "deal 4, doubled if the
> target is against a wall", "costs 0 if you haven't moved this turn". This is
> the richest unexplored vein, because it makes positioning matter even on cards
> that aren't movement cards.

### 3.5 Status effects **[next]**

Let's keep this small for now. Four, maximum, for the slice:

| Status | Effect |
|---|---|
| Weak | Target deals reduced damage |
| Vulnerable | Target takes increased damage |
| Rooted | Target cannot move (free step or cards) |
| Burn | Damage at end of turn, decays |

*Rooted* is the one that's genuinely novel here — a status that attacks the
positioning layer rather than the damage layer. Worth leaning on.

### 3.6 Enemies and intents **[built]**

Every enemy displays, each turn: what it will do, how much, and **which tiles it
threatens**. Threatened tiles are shaded on the grid.

This is the demo's hook. It's what makes the game read as tactical in a
screenshot rather than needing to be explained.

Enemy archetypes to build:

| Archetype | Role | Why it exists |
|---|---|---|
| Chip Rat | Closes distance, bites adjacent | Teaches: kill it or step away |
| Slugger | Slow, telegraphs a big 3-tile area | Teaches: read the shading and move |
| Spitter | Ranged, threatens a full line, doesn't approach | Teaches: break line of sight with walls |
| Anchor | Doesn't move; buffs and roots | Teaches: priority targeting |
| Elite | Two intents per turn | Pressure test |
| Boss | Phases + spawns adds | Finale |

AI is **not** a search. Each enemy has a short ordered list of abilities with
conditions ("if adjacent → bite; else → step toward nearest player"). Picks the
first that applies, telegraphs it, executes next turn. Predictability is a
feature: pillar 2 requires the player be able to reason about what happens next.

### 3.7 Combat math (starting point, expect to retune)

| | |
|---|---|
| Player HP | 30 |
| Normal enemy HP | 10–16 |
| Elite HP | 25–35 |
| Basic attack | 5–7 |
| Enemy hit | 4–8 |
| Fight length | 4–6 turns |
| Enemies per fight | 2–3 normal · 3–4 elite · boss + adds |

Target feel: a normal enemy dies to roughly two committed cards. A fight the
player plays perfectly should still cost some HP, because HP is the run-level
resource that makes rest nodes a real choice.

### 3.8 Win / lose

All enemies dead → victory → reward. Player at 0 HP → run ends.

---

## 4. Deck and cards

### Starting deck — fixed, 10 cards **[next]**

No draft at run start. A fixed deck means every run opens from the same known
position, so the *map* is the first real decision, and it makes card pickups
legible against a stable baseline.

| Count | Card | Sketch |
|---|---|---|
| 4 | Jab | 1 energy · adjacent · 5 damage |
| 3 | Guard | 1 energy · self · 5 block |
| 2 | Reposition | 1 energy · dash up to 3 |
| 1 | Sweep | 2 energy · cone 2 in a direction · 4 damage |

Enough to demonstrate melee, defence, movement and a shaped attack in the first
fight, without explaining anything.

### Acquisition

- Reward after each combat: pick 1 of 3
- Shop: buy cards, remove cards
- Events: sometimes offer strange ones

**Card removal matters more than card addition** in a short run — worth making
it available and slightly expensive.

### Upgrades **[next]**

Rest nodes upgrade a card. An upgrade is a numeric bump or a cost reduction on
the same definition, not a new card.

> **[idea]** Upgrades that change the *shape* rather than the number — Sweep's
> cone going from 2 to 3, or Jab gaining range 2. Far more interesting for this
> game specifically than "+2 damage", because it changes where you must stand.
> Probably the single best idea in this document.

---

## 5. The run map **[built]**

5–8 nodes to a boss. Branching, FTL/StS style — rows of nodes with edges only to
nearby nodes in the next row, so a choice is a commitment for several steps.

| Node | Purpose |
|---|---|
| Combat | Baseline |
| Elite | Higher risk, better reward |
| Event | Text choice, unpredictable |
| Rest | Heal **or** upgrade — never both |
| Shop | Convert gold; the only place to remove cards |
| Boss | Terminal |

Generation: pick a fixed number of paths from the bottom row, walk each upward
choosing among adjacent nodes in the next row, merging where they collide. Then
assign types under constraints (no two shops adjacent, elite not on row 1, rest
never immediately before the boss... or *always* before the boss? — see open
questions).

Every node must be a real choice. If one option is strictly better, the map
isn't doing its job.

---

## 6. Meta-progression **[built, minimal]**

Deliberately thin. This is a vertical slice; a deep meta layer would be scope
creep that adds nothing to the demo.

- Unlock a small pool of extra cards by completing runs / hitting milestones
- Track basic stats: runs, wins, furthest node
- Persist to a single small file

That's the whole thing.

> **[idea]** Map modifiers unlocked between runs (a "harder but richer" mode).
> **[idea]** Ascension-style difficulty ladder. Both are post-portfolio.

---
