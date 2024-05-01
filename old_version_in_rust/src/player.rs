use soloud::*;
use drylib::*;

use std::{
    thread::{self, JoinHandle},
    sync::mpsc::{Sender, Receiver}
};

use crate::action::Action;

pub(crate) type AM<T> = std::sync::Arc::<std::sync::Mutex::<T>>;

pub(crate) struct Player {
    sl:        AM::<Soloud>,
    sl_han:    AM::<Option<Handle>>,
    back_sx:   AM::<Sender<f64>>,
    action_rx: AM::<Receiver<Action>>,
}

impl Player {
    pub(crate) fn new
    (
        sl:        AM::<Soloud>,
        sl_han:    AM::<Option<Handle>>,
        back_sx:   AM::<Sender<f64>>,
        action_rx: AM::<Receiver<Action>>
    ) -> Player
    {
        Player {
            sl,
            sl_han,
            back_sx,
            action_rx
        }
    }

    pub(crate) fn run(self, wav: Wav) -> JoinHandle<()> {
        let len = wav.length();
        let out_han = self.run_output(wav);
        self.run_input(len, out_han)
    }

    fn run_output(&self, wav: Wav) -> JoinHandle<()> {
        let csl      = self.sl.clone();
        let csl_han  = self.sl_han.clone();
        let cback_sx = self.back_sx.clone();
        thread::spawn(move || {
            *lock!(csl_han) = Some(lock!(csl).play(&wav));
            let back_sx = lock!(cback_sx);
            loop {
                back_sx.send(1.0).unwrap();
                sleep!(i|1);
            }
        })
    }

    fn run_input(&self, len: f64, out_han: JoinHandle<()>) -> JoinHandle<()> {
        let csl        = self.sl.clone();
        let csl_han    = self.sl_han.clone();
        let caction_rx = self.action_rx.clone();
        thread::spawn(move || {
            let mut pause = false;
            loop {
                if let (Ok(action), Ok(mut sl), Some(han)) =
                    (lock!(caction_rx).try_recv(),
                     csl.lock(),
                     *csl_han.lock().unwrap())
                {
                    if action == Action::Exit {
                        sl.stop(han);
                        out_han.join().unwrap();
                        break;
                    }
                    Player::handle_action(&mut pause, &mut sl, &len, &han, &action);
                }
            }
        })
    }

    fn handle_action
    (
        pause:  &mut bool,
        sl:     &mut Soloud,
        len:    &f64,
        han:    &Handle,
        action: &Action
    )
    {
        match action {
            Action::SeekForward(sec)   => {
                let position = sl.stream_position(*han);
                sl.seek(*han, len.min(position + sec)).unwrap();
            }
            Action::SeekBackward(sec)  => {
                let position = sl.stream_position(*han);
                sl.seek(*han, (0.0f64).max(position - sec)).unwrap();
            }
            Action::Pause => {
                *pause = !*pause;
                sl.set_pause(*han, *pause);
            }
            Action::SetVol(vol) => sl.set_volume(*han, *vol),
            _ => {}
        }        
    }
}
