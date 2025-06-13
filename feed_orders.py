#!/usr/bin/env python3
import socket
import time
import random
import argparse

def iso_ns_timestamp():
    # nanoseconds since epoch
    return int(time.time() * 1e9)

def format_order(order_id, account_id, symbol, side, otype, price, qty):
    # CSV: orderId,accountId,symbol,side,type,price,quantity,timestamp\n
    return f"{order_id},{account_id},{symbol},{side},{otype},{price:.2f},{qty},{iso_ns_timestamp()}\n"

def main():
    p = argparse.ArgumentParser(description="Feed simulated orders into the matching engine")
    p.add_argument("--host", default="localhost", help="Engine host")
    p.add_argument("--port", type=int, default=9000, help="Engine port")
    p.add_argument("--symbols", default="AAPL,GOOG,TSLA",
                   help="Comma-separated list of symbols")
    p.add_argument("--rate", type=float, default=1.0,
                   help="Orders per second")
    p.add_argument("--limit", type=int, default=0,
                   help="Total orders to send (0 = infinite)")
    args = p.parse_args()

    symbols = args.symbols.split(",")
    interval = 1.0 / args.rate
    order_id = 1

    with socket.create_connection((args.host, args.port)) as sock:
        print(f"Connected to {args.host}:{args.port}, streaming orders at {args.rate} Hz")
        while True:
            sym    = random.choice(symbols)
            side   = random.choice([0,1])
            otype  = random.choice([0,1])
            price  = random.uniform(100, 200) if otype==0 else 0.0
            qty    = random.randint(1, 10)
            acct   = random.randint(1, 5)
            line   = format_order(order_id, acct, sym, side, otype, price, qty)
            sock.sendall(line.encode())
            order_id += 1

            if args.limit and order_id > args.limit:
                break
            time.sleep(interval)

if __name__ == "__main__":
    main()