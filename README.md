# ohbobispft — Terminal Planner

A CLI planner written in C. Create and view plans for your days, weeks,
months, and years from the terminal with an interactive TUI grid.

## Install

### Prerequisites

- GCC (or any C11 compiler)
- Make
- Linux (uses `termios`, `ioctl`)
- An editor (defaults to `$EDITOR`, else nvim/vim/vi)

### Build

```sh
git clone <repo-url> && cd bobispft
make
```

This produces the `ohbobispft` binary in the current directory.

### Add to PATH

**Option A — copy to a directory already on PATH:**

```sh
sudo cp ohbobispft /usr/local/bin/
```

**Option B — add the project directory to PATH (add to `~/.bashrc` or `~/.zshrc`):**

```sh
export PATH="$PATH:/path/to/bobispft"
```

Then reload your shell:

```sh
source ~/.bashrc   # or source ~/.zshrc
```

Verify it works:

```sh
ohbobispft --version
```

## Usage

### Create a plan

```sh
ohbobispft np
```

Prompts for a date (default: tomorrow), a title, then opens your editor
(`$EDITOR`) to write the plan body. If a plan already exists for that
date/scope, it shows the existing plan and asks whether to overwrite.

**Scope flags** choose what period the plan covers:

```sh
ohbobispft np -w       # weekly plan (current week, starting Sunday)
ohbobispft np -m       # monthly plan (current month)
ohbobispft np -y       # yearly plan  (current year)
```

**Plan for a specific date:**

```sh
ohbobispft np -d 2026-06-28
```

The `-d` date is interpreted per scope: for `-w` it picks the week
containing that date; for `-m` the month; for `-y` the year.

### View plans

```sh
ohbobispft sp
```

Opens an interactive TUI grid. Navigate with:

| Key | Action |
|---|---|
| `i` | Cycle view (daily → weekly → monthly → yearly) |
| `n` | Next (day / week / month / year) |
| `N` | Previous (day / week / month / year) |
| `←` `→` | Previous / next day or week |
| `e` | Edit the plan for the current view's scope |
| `q` | Quit |

The yearly view shows a 4×3 grid of months with months that have a
plan highlighted. The monthly view shows a calendar with days that have
a plan highlighted. Weekly and monthly views also show the week/month
plan title as a header when one exists.

**Start at a specific date:**

```sh
ohbobispft sp -d 2026-06-17
```

### Edit an existing plan

```sh
ohbobispft ep              # edit tomorrow's day plan (prompts for date)
ohbobispft ep -d 2026-06-28
ohbobispft ep -w           # edit this week's plan
ohbobispft ep -m           # edit this month's plan
ohbobispft ep -y           # edit this year's plan
```

### Supported date formats

- `2026-06-17` (ISO)
- `June 17, 2026` (US long)
- `17 Jun 2026` (European)
- `today`, `tomorrow`
- `next monday`, `next tuesday`, etc.

## Run tests

```sh
make test
```

## Project structure

```
├── main.c              Entry point
├── core/               Business logic
│   ├── include/        Headers
│   ├── src/            Implementation
│   └── tests/          Unit tests
├── renderer/           Terminal UI
│   ├── include/
│   ├── src/
│   └── tests/
└── data/               Plan storage (.bobip files)
```

## Data storage

Plans are saved as binary files in `data/`, one `.bobip` file per plan.
Each scope uses a distinct filename prefix:

- Day:   `d-YYYY-MM-DD.bobip`
- Week:  `w-YYYY-MM-DD.bobip` (Sunday-start date of the week)
- Month: `m-YYYY-MM.bobip`
- Year:  `y-YYYY.bobip`

The file format includes a magic header (`PLAN`), version byte, a scope
byte, an XOR-8 checksum, and the packed title + content. Files from disk
are treated as untrusted: short reads, forged lengths, bad dates, and
mismatched scopes are all rejected on load.
