use crossterm::{execute, terminal::*};
use ratatui::{Terminal, prelude::CrosstermBackend};

use std::io::{stdout, Stdout};

use crate::ui::IResult;

pub(crate) type Tui = Terminal<CrosstermBackend<Stdout>>;

pub fn init() -> IResult<Tui> {
    execute!(stdout(), EnterAlternateScreen)?;
    enable_raw_mode()?;
    Terminal::new(CrosstermBackend::new(stdout()))
}

pub fn restore() -> IResult<()> {
    execute!(stdout(), LeaveAlternateScreen)?;
    disable_raw_mode()?;
    Ok(())
}
