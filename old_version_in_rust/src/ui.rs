use crossterm::event::{self, *, KeyEventKind::Press};
use ratatui::{
    Frame,
    widgets::*,
    style::Style,
    prelude::{Span, Alignment, Color},
    layout::{Constraint, Direction, Layout},
};

use std::{
    sync::mpsc::{Sender, Receiver},
    time::Duration        
};

pub(crate) type IResult<T> = std::io::Result::<T>;

use crate::{tui, action::Action};

pub(crate) struct UI {
    music_vol: f64,
    music_pos: f64,
    music_len: f64,
    music_name: String,
    action_sx: Sender<Action>,
    back_rx: Receiver<f64>,
    local_exit: bool,
    local_pause: bool
}

impl UI {
    pub(crate) fn new
    (
        music_vol: f64,
        music_len: f64,
        music_name: String,
        action_sx: Sender<Action>,
        back_rx: Receiver<f64>
    ) -> UI {
        UI {
            music_vol,
            music_pos: 0.0,
            music_len,
            music_name,
            action_sx,
            back_rx,
            local_exit: false,
            local_pause: false
        }
    }

    pub(crate) fn run(&mut self, t: &mut tui::Tui) -> IResult<()> {
        while !self.local_exit {
            if let Ok(pos) = self.back_rx.try_recv() {
                if !self.local_pause {
                    if self.music_pos == self.music_len {
                        self.music_pos = 0.0;
                    }
                    self.music_pos = (self.music_pos + pos).min(self.music_len)
                }
            }
            t.draw(|f| self.render_frame(f))?;
            if event::poll(Duration::from_millis(10))? {
                self.handle_events(event::read()?)?;
            }
        }
        self.action_sx.send(Action::Exit).unwrap();
        Ok(())
    }

    fn render_frame(&self, f: &mut Frame) {
        let main_chunks = Layout::default()
            .direction(Direction::Vertical)
            .constraints([
                Constraint::Percentage(80),
                Constraint::Percentage(20),
            ]).split(f.size());
        
        let bottom_chunks = Layout::default()
            .direction(Direction::Horizontal)
            .constraints([
                Constraint::Percentage(60),
                Constraint::Percentage(40),
            ]).split(main_chunks[1]);

        let song_paragraph = Paragraph::new(
            Span::from(
                Span::styled(
                    format!("Song playing: {m_name}", m_name = self.music_name),
                    Style::default().fg(Color::White).underline_color(Color::Cyan),
                )
            )).alignment(Alignment::Center)
            .block(Block::default().borders(Borders::ALL).title("Information"));
        
        let vol_gauge = Gauge::default()
            .block(Block::default().borders(Borders::ALL).title("Sound Level"))
            .gauge_style(Style::default().fg(Color::Green))
            .ratio(self.music_vol);

        let music_pos_text = format!("{:.2}/{:.2} seconds", self.music_pos, self.music_len);
        let pos_gauge = Gauge::default()
            .block(Block::default().borders(Borders::ALL).title("Music Position"))
            .gauge_style(Style::default().fg(Color::Green))
            .ratio(self.music_pos / self.music_len)
            .label(music_pos_text);

        f.render_widget(song_paragraph, main_chunks[0]);
        f.render_widget(pos_gauge, bottom_chunks[0]);
        f.render_widget(vol_gauge, bottom_chunks[1]);
    }

    fn handle_events(&mut self, event: Event) -> IResult<()> {
        match event {
            Event::Key(k_event) if k_event.kind == Press => {
                match k_event.code {
                    KeyCode::Char('q') => self.local_exit = true,
                    KeyCode::Char(' ') => {
                        self.local_pause = !self.local_pause;
                        self.action_sx.send(Action::Pause).unwrap()
                    }
                    KeyCode::Right     => {
                        self.music_pos = (self.music_pos + 5.0).min(self.music_len);
                        self.action_sx.send(Action::SeekForward(5.0)).unwrap()
                    }
                    KeyCode::Left      => {
                        self.music_pos = (self.music_pos - 5.0).max(0.0);
                        self.action_sx.send(Action::SeekBackward(5.0)).unwrap()
                    }
                    KeyCode::Up        => {
                        self.music_vol = (self.music_vol + 0.05).min(1.0);
                        self.action_sx.send(Action::SetVol(self.music_vol as f32)).unwrap();
                    }
                    KeyCode::Down      => {
                        self.music_vol = (self.music_vol - 0.05).max(0.0);
                        self.action_sx.send(Action::SetVol(self.music_vol as f32)).unwrap();
                    }
                    _ => {}
                }
            }
            _ => {}
        }
        Ok(())
    }
}
