use std::io::prelude::*;

const RESP_OK: &[u8] = b"HTTP/1.1 200 OK\r\n\r\n";

const RESP_NOT_FOUND: &[u8] = b"HTTP/1.1 404 Not Found\r\n\r\n";

const RESP_METHOD_NOT_ALLOWED: &[u8] =
    b"HTTP/1.1 405 Method Not Allowed\r\n\r\n";

const WEBSOCKET_HANDSHAKE: &[u8] = b"\
HTTP/1.1 101 Switching Protocols\r
Upgrade: websocket\r
Connection: Upgrade\r
Sec-WebSocket-Accept: ";

const FIN_BIT: u8 = 0b1000_0000;
const OPCODE_MASK: u8 = 0b1111;
const OP_TEXT: u8 = 0x1;
const OP_CLOSE: u8 = 0x8;
const MASK_BIT: u8 = 0b1000_0000;
const LEN_MASK: u8 = 0b0111_1111;

struct Options {
    verbose: bool,
    address: String,
    index_path: String,
}

fn main() -> std::io::Result<()> {
    let opt = parse_args();
    if opt.index_path.is_empty() {
        panic!("required argument: index`");
    }
    let index = std::fs::read_to_string(opt.index_path)?;
    let listener = std::net::TcpListener::bind(opt.address)?;
    let (send, recv) = std::sync::mpsc::channel::<Vec<u8>>();
    let worker = start_worker(recv, opt.verbose);
    let mut ctx = Context {
        verbose: opt.verbose,
        index: index.as_bytes(),
        ch: &send,
    };
    for stream in listener.incoming() {
        handle_req(stream?, &mut ctx)?
    }
    drop(send);
    worker.join().expect("failed to join worker thread")
}

fn parse_args() -> Options {
    let mut verbose = false;
    let mut address = String::from("localhost:8000");
    let mut index_path = String::new();
    let mut pos = [&mut address, &mut index_path];
    let mut pos_it = pos.iter_mut();
    for arg in std::env::args().skip(1) {
        match arg.as_str() {
            "-v"|"--verbose" => verbose = true,
            _ => match pos_it.next() {
                None => panic!("extra argument: {}", arg),
                Some(x) => **x = arg,
            }
        }
    }
    Options { verbose, address, index_path }
}

fn start_worker(
        ch: std::sync::mpsc::Receiver<Vec<u8>>, verbose: bool)
        -> std::thread::JoinHandle<std::io::Result<()>> {
    std::thread::spawn(move || {
        let mut out = std::io::stdout();
        ch.iter().map(|msg| {
            if verbose {
                eprintln!("msg: {}", String::from_utf8_lossy(&msg));
            }
            out.write_all(&msg)?;
            out.flush()
        }).collect()
    })
}

struct Context<'a> {
    verbose: bool,
    index: &'a [u8],
    ch: &'a std::sync::mpsc::Sender::<Vec<u8>>,
}

fn handle_req<T>(mut stream: T, ctx: &mut Context)
        -> std::io::Result<()>
        where T: 'static + Read + Write + Send {
    let mut buf = [0u8; 1024];
    stream.read(&mut buf)?;
    let req = std::str::from_utf8(&buf).unwrap();
    let (method, path) = {
        let mut it = req.split(' ');
        (it.next().unwrap_or(""), it.next().unwrap_or(""))
    };
    if method != "GET" {
        stream.write_all(RESP_METHOD_NOT_ALLOWED)?;
        return stream.flush();
    }
    if ctx.verbose {
        eprintln!("request:\n{}", req);
    }
    if split_query(path).0 != "/" {
        stream.write_all(RESP_NOT_FOUND)?;
        return stream.flush();
    }
    let headers = parse_headers(req);
    if headers.iter().find(|&&x| x == ("Upgrade", "websocket")) == None {
        stream.write_all(RESP_OK)?;
        stream.write_all(ctx.index)?;
        return stream.flush()
    }
    ws_handshake(&mut stream, &headers, ctx.verbose).map(|_| {
        let (v, ch) = (ctx.verbose, ctx.ch.clone());
        std::thread::spawn(move || handle_ws(&mut stream, ch, v));
    })
}

fn split_query(path: &str) -> (&str, &str) {
    match path.find('?') {
        Some(i) => (&path[..i], &path[i + 1..]),
        None => (path, ""),
    }
}

fn parse_headers(req: &str) -> Vec<(&str, &str)> {
    req.split("\r\n\r\n")
        .next()
        .unwrap_or("")
        .lines()
        .skip(1)
        .map(|x| {
            let mut it = x.splitn(2, ':');
            let mut v = || it.next().unwrap_or_default();
            (v(), v().trim_start())
        })
        .collect()
}

fn ws_handshake<T: Read + Write>(
        stream: &mut T,
        headers: &[(&str, &str)], verbose: bool)
        -> std::io::Result<()> {
    let key = headers.iter()
        .find(|x| x.0 == "Sec-WebSocket-Key")
        .map(|x| x.1)
        .unwrap_or_default();
    let resp_key = ws_key(key);
    if verbose {
        eprintln!(
            "handshake:{}{}",
            String::from_utf8_lossy(WEBSOCKET_HANDSHAKE),
            String::from_utf8_lossy(&resp_key));
    }
    stream.write_all(WEBSOCKET_HANDSHAKE)?;
    stream.write_all(&resp_key)?;
    stream.write_all(b"\r\n\r\n")?;
    stream.flush()
}

fn ws_key(key: &str) -> Vec<u8> {
    let sha1 = std::process::Command::new("openssl")
        .arg("sha1").arg("--binary")
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .spawn().unwrap();
    let b64 = std::process::Command::new("base64")
        .arg("--wrap").arg("0")
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .spawn().unwrap();
    let mut stdin = sha1.stdin.unwrap();
    stdin.write_all(key.as_bytes()).unwrap();
    stdin.write_all(b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11").unwrap();
    drop(stdin);
    let mut b = Vec::new();
    sha1.stdout.unwrap().read_to_end(&mut b).unwrap();
    b64.stdin.unwrap().write_all(&b).unwrap();
    b.clear();
    b64.stdout.unwrap().read_to_end(&mut b).unwrap();
    b
}

fn handle_ws<T: 'static + Read + Write + Send>(
        stream: &mut T,
        ch: std::sync::mpsc::Sender<Vec<u8>>,
        verbose: bool) {
    let mut header = [0u8; 2];
    loop {
        stream.read_exact(&mut header).unwrap();
        let fin = header[0] & FIN_BIT != 0;
        let opcode = header[0] & OPCODE_MASK;
        let mask = header[1] & MASK_BIT != 0;
        let len = header[1] & LEN_MASK;
        if verbose {
            eprintln!("websocket:");
            eprintln!("raw: {} {}", header[0], header[1]);
            eprintln!("fin: {}, opcode: {}, mask: {}", fin, opcode, mask);
            eprintln!("{}", match opcode {
                0 => "continuation",
                1 => "text",
                2 => "binary",
                _ => "other",
            });
            eprintln!("len: {}", len);
        }
        if !fin || len == 125 {
            unimplemented!();
        }
        match opcode {
            OP_CLOSE => {
                stream.write(&[FIN_BIT | OP_CLOSE, 0u8])
                    .expect("failed to send close frame");
                break
            },
            OP_TEXT => (),
            _ => unimplemented!(),
        }
        let mut mask_key = [0u8; 4];
        if mask {
            stream.read_exact(&mut mask_key).unwrap();
        }
        if verbose {
            eprintln!(
                "mask_key: {} {} {} {}",
                mask_key[0], mask_key[1], mask_key[2], mask_key[3]);
        }
        let mut data = Vec::new();
        data.resize(len as usize, 0);
        stream.read_exact(&mut data).unwrap();
        for (i, x) in data.iter_mut().enumerate() {
            *x ^= mask_key[i % 4];
        }
        if verbose {
            eprintln!("data: {}", String::from_utf8_lossy(&data));
        }
        ch.send(data).unwrap();
    }
}

#[cfg(test)]
mod tests {
    use super::FIN_BIT;
    use super::OP_TEXT;
    use super::OP_CLOSE;
    use super::MASK_BIT;
    use super::Context;
    use super::handle_req;

    #[test]
    fn split_query() {
        use super::split_query;
        assert_eq!(split_query("/no_query"), ("/no_query", ""));
        assert_eq!(split_query("/empty?"), ("/empty", ""));
        assert_eq!(split_query("/query?the=query"), ("/query", "the=query"));
    }

    #[test]
    fn parse_headers_empty() {
        let ret = super::parse_headers("");
        assert_eq!(ret, vec![]);
        let ret = super::parse_headers("HTTP/1.1 GET /\r\n\r\n");
        assert_eq!(ret, vec![]);
    }

    #[test]
    fn parse_headers() {
        let ret = super::parse_headers("\
HTTP/1.1 GET /\r
Some: header\r
Another:   header with leading spaces\r\n\r\n");
        assert_eq!(ret, vec![
            ("Some", "header"),
            ("Another", "header with leading spaces"),
        ]);
    }

    struct TestStream {
        r: std::io::Cursor<Vec<u8>>,
        w: std::sync::Arc<std::sync::Mutex<Vec<u8>>>,
    }

    impl TestStream {
        fn new(r: Vec<u8>) -> TestStream {
            TestStream {
                r: std::io::Cursor::new(r),
                w: std::sync::Arc::new(std::sync::Mutex::new(Vec::new())),
            }
        }

        fn output(w: &std::sync::Arc<std::sync::Mutex<Vec<u8>>>) -> Vec<u8> {
            w.try_lock().unwrap().clone()
        }

        fn str_output(
            w: &std::sync::Arc<std::sync::Mutex<Vec<u8>>>,
        ) -> String {
            String::from_utf8_lossy(&w.try_lock().unwrap()).to_string()
        }
    }

    impl std::io::Read for TestStream {
        fn read(&mut self, b: &mut [u8]) -> std::io::Result<usize> {
            self.r.read(b)
        }
    }

    impl std::io::Write for TestStream {
        fn write(&mut self, b: &[u8]) -> std::io::Result<usize> {
            self.w.lock().unwrap().write(b)
        }

        fn flush(&mut self) -> std::io::Result<()> {
            self.w.lock().unwrap().flush()
        }
    }

    #[test]
    fn handle_req_root() -> std::io::Result<()>{
        let stream = TestStream::new(b"GET / HTTP/1.0\r\n\r\n".to_vec());
        let output = stream.w.clone();
        let (ch, _) = std::sync::mpsc::channel::<Vec<u8>>();
        let mut ctx = Context{ verbose: true, index: b"content", ch: &ch };
        handle_req(stream, &mut ctx)?;
        assert_eq!(
            &TestStream::str_output(&output),
            "HTTP/1.1 200 OK\r\n\r\ncontent");
        Ok(())
    }

    #[test]
    fn handle_req_post() -> std::io::Result<()>{
        let stream = TestStream::new(b"POST / HTTP/1.0\r\n\r\n".to_vec());
        let output = stream.w.clone();
        let (ch, _) = std::sync::mpsc::channel::<Vec<u8>>();
        let mut ctx = Context{ verbose: true, index: b"", ch: &ch };
        handle_req(stream, &mut ctx)?;
        assert_eq!(
            &TestStream::str_output(&output),
            "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
        Ok(())
    }

    #[test]
    fn handle_req_sub() -> std::io::Result<()>{
        let stream = TestStream::new(b"GET /sub HTTP/1.0\r\n\r\n".to_vec());
        let output = stream.w.clone();
        let (ch, _) = std::sync::mpsc::channel();
        let mut ctx = Context{ verbose: true, index: b"", ch: &ch };
        handle_req(stream, &mut ctx)?;
        assert_eq!(
            &TestStream::str_output(&output),
            "HTTP/1.1 404 Not Found\r\n\r\n");
        Ok(())
    }

    #[test]
    fn ws_handshake() -> std::io::Result<()> {
        let mut stream = TestStream::new(b"".to_vec());
        super::ws_handshake(
            &mut stream, &[("Sec-WebSocket-Key", "dGVzdA==")], true)?;
        assert_eq!(
            &TestStream::str_output(&stream.w),
            "\
HTTP/1.1 101 Switching Protocols\r
Upgrade: websocket\r
Connection: Upgrade\r
Sec-WebSocket-Accept: T3TQpOmnRqmZUfV9OrgZq2FJw54=\r\n\r\n");
        Ok(())
    }

    #[test]
    fn ws_key() {
        let f = |x| String::from_utf8(super::ws_key(x)).unwrap();
        assert_eq!(f("test0"), "4lAXywWR6DL+ZXTWPjAgSWCKq9U=");
        assert_eq!(f("test1"), "4RZrzunikRkJcfTKOVCtDTZUTec=");
    }

    #[test]
    fn handle_ws() {
        let mask = b"\x00\x01\x02\x03";
        let mut stream = {
            let mut v = Vec::new();
            v.push(FIN_BIT | OP_TEXT);
            v.push(MASK_BIT | 5u8);
            v.extend(mask);
            v.extend(b"\x74\x69\x6b\x70\x20");
            v.push(FIN_BIT | OP_TEXT);
            v.push(MASK_BIT | 3u8);
            v.extend(mask);
            v.extend(b"\x69\x72\x22");
            v.push(FIN_BIT | OP_TEXT);
            v.push(MASK_BIT | 2u8);
            v.extend(mask);
            v.extend(b"\x61\x21");
            v.push(FIN_BIT | OP_TEXT);
            v.push(MASK_BIT | 4u8);
            v.extend(mask);
            v.extend(b"\x74\x64\x71\x77");
            v.push(FIN_BIT | OP_CLOSE);
            v.push(0u8);
            TestStream::new(v)
        };
        let (send, recv) = std::sync::mpsc::channel();
        super::handle_ws(&mut stream, send, true);
        assert_eq!(String::from_utf8(recv.recv().unwrap()).unwrap(), "this ");
        assert_eq!(String::from_utf8(recv.recv().unwrap()).unwrap(), "is ");
        assert_eq!(String::from_utf8(recv.recv().unwrap()).unwrap(), "a ");
        assert_eq!(String::from_utf8(recv.recv().unwrap()).unwrap(), "test");
        assert_eq!(
            &TestStream::output(&stream.w),
            &vec![FIN_BIT | OP_CLOSE, 0u8]);
    }
}
