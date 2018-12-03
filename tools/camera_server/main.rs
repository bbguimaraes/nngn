use std::io::prelude::*;

fn main() -> std::io::Result<()> {
    let mut vs = (&mut Vec::new(), &mut Vec::new(), &mut Vec::new());
    let mut dst: Box<dyn Write> = match std::env::args().nth(1) {
        None => Box::new(std::io::stdout()),
        Some(x) => Box::new(std::os::unix::net::UnixStream::connect(x)?),
    };
    std::io::BufReader::new(std::io::stdin()).lines().map(
        |x| x.and_then(|line| {
            let v = read_values(&line);
            if v.0.is_nan() || v.1.is_nan() || v.2.is_nan() {
                return Ok(());
            }
            let v = add_values(&v, 5, &mut vs);
            let msg = format!(
                "require(\"nngn.lib.camera\").get():set_rot({}, {}, {})\n",
                v.0.to_radians(), v.1.to_radians(), v.2.to_radians());
            dst.write_all(msg.as_bytes())?;
            dst.flush()
    })).collect()
}

fn read_values(v: &str) -> (f32, f32, f32) {
    let mut it = v
        .split_whitespace()
        .map(|x| x.parse::<f32>().unwrap_or(0.0))
        .take(3);
    return (
        it.next().unwrap_or(0.0),
        it.next().unwrap_or(0.0),
        it.next().unwrap_or(0.0));
}

fn add_values(
        xs: &(f32, f32, f32), n: usize,
        vs: &mut (&mut Vec<f32>, &mut Vec<f32>, &mut Vec<f32>))
        -> (f32, f32, f32) {
    return (
        add_value(xs.0, n, vs.0),
        add_value(xs.1, n, vs.1),
        add_value(xs.2, n, vs.2));
}

fn add_value(x: f32, n: usize, v: &mut Vec<f32>) -> f32 {
    let sx = smooth(x, n, v);
    while v.len() >= n {
        v.remove(0);
    }
    v.push(sx);
    sx
}

fn smooth(v: f32, n: usize, l: &Vec<f32>) -> f32 {
    let fac = (1..=n)
        .zip(l.iter().rev())
        .map(|(i, x)| x * i as f32)
        .sum::<f32>();
    let div = ((n + n * n) as f32) / 2.0;
    let v_fac = (n + 1) as f32;
    (v * v_fac + fac) / (v_fac + div)
}
