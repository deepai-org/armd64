import os
import selectors
import socket
import sys


def main():
    selector = selectors.DefaultSelector()
    if selector.__class__.__name__ != "EpollSelector":
        print("POLY_PYTHON_EPOLL_FAIL: selector=%s" %
              selector.__class__.__name__, flush=True)
        return 3

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    server.setblocking(False)
    selector.register(server, selectors.EVENT_READ, "accept")

    idle_waits = 0
    for _ in range(3):
        if selector.select(0.02):
            return 4
        idle_waits += 1

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.setblocking(False)
    try:
        client.connect(server.getsockname())
    except BlockingIOError:
        pass
    selector.register(client, selectors.EVENT_WRITE, "client-write")

    response = bytearray()
    accepted = None
    served = False

    for _ in range(64):
        events = selector.select(0.1)
        if not events:
            continue
        for key, mask in events:
            role = key.data
            sock = key.fileobj
            if role == "accept":
                accepted, _ = sock.accept()
                accepted.setblocking(False)
                selector.register(accepted, selectors.EVENT_READ, "server-read")
            elif role == "client-write":
                sock.sendall(b"GET / HTTP/1.1\r\nHost: poly\r\n\r\n")
                selector.modify(sock, selectors.EVENT_READ, "client-read")
            elif role == "server-read":
                request = sock.recv(4096)
                if b"GET /" in request:
                    selector.unregister(sock)
                    sock.sendall(
                        b"HTTP/1.1 200 OK\r\n"
                        b"Content-Length: 13\r\n\r\n"
                        b"poly-epoll-ok")
                    sock.close()
                    served = True
            elif role == "client-read":
                chunk = sock.recv(4096)
                if chunk:
                    response.extend(chunk)
                if b"poly-epoll-ok" in response:
                    selector.unregister(sock)
                    sock.close()
                    selector.unregister(server)
                    server.close()
                    selector.close()
                    print("POLY_PYTHON_EPOLL_OK: selector=EpollSelector "
                          "idle_waits=%d uid=%d served=%d" %
                          (idle_waits, os.getuid(), served), flush=True)
                    return 42

    if accepted is not None:
        accepted.close()
    client.close()
    server.close()
    selector.close()
    print("POLY_PYTHON_EPOLL_FAIL: timeout", flush=True)
    return 6


if __name__ == "__main__":
    sys.exit(main())
