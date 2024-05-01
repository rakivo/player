use drylib::*;
use soloud::*;

use std::{
    env,
    thread,
    sync::mpsc,
    path::PathBuf
};

mod ui;
mod tui;
mod player;
mod action;

const DELIM: char = if cfg!(windows) { '\\' } else { '/' };
const EXTENSIONS: &[&str] = &[".wav", ".mp3", ".ogg", ".flac"];

fn main() -> ui::IResult<()> {
    let args = env::args().collect::<Vec<_>>();
    assert!(args.len() == 2, "USAGE: {prog} <file path>", prog = args[0]);
    
    let path = {
        let home = env::var("HOME").expect("Could not get HOME directory");
        let path = args[1].replacen("~", &home, 1);
        let meta = PathBuf::from(&path).metadata()?;
        if meta.is_file() && EXTENSIONS.iter().any(|ext| path.ends_with(ext)) {
            path
        } else {
            panic!("Supported extensions: {EXTENSIONS:?}");
        }
    };

    let (back_sx, back_rx) = mpsc::channel::<f64>();
    let (action_sx, action_rx) = mpsc::channel::<action::Action>();

    let sl = Soloud::default().unwrap();
    let player = player::Player::new(am!(sl), am!(None), am!(back_sx), am!(action_rx));

    let music_name = path.chars()
        .rev()
        .take_while(|x| *x != DELIM)
        .collect::<String>()
        .chars()
        .rev() // reverse string back
        .collect::<String>();

    let mut wav = Wav::from_path(path).unwrap();
    wav.set_volume(0.05);
    wav.set_looping(true);

    let player_len = wav.length();
    let player_handler = thread::spawn(move || player.run(wav));

    let mut t = tui::init()?;
    let mut ui = ui::UI::new(0.05, player_len, music_name, action_sx, back_rx);

    ui.run(&mut t).ok();
    player_handler.join().unwrap();
    tui::restore()
}
