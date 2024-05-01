#[derive(PartialEq)]
pub(crate) enum Action {
    SetVol(f32),
    SeekForward(f64),
    SeekBackward(f64),
    Pause,
    Exit
}
