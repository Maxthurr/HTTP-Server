#!/usr/bin/env python3

import os
import shutil
import subprocess as sp
import socket
import requests
import http
import http.client
import pytest
import time

HOST = "127.0.0.1"
PORT = "8080"
BIN = os.environ.get('HTTPD_BIN', "../../httpd")
ROOT_DIR = "../test_game/"


def setup(stdout_filename, args=[]):
    global BIN
    # if BIN not present, try PATH
    if not (os.path.isfile(BIN) and os.access(BIN, os.X_OK)):
        which = shutil.which('httpd')
        if which:
            BIN = which
        else:
            pytest.skip('httpd binary not found; set HTTPD_BIN or build httpd')

    # open stdout in write mode
    out_f = open(stdout_filename, 'w')
    process = sp.Popen([BIN, "--pid-file", "/tmp/HTTPd.pid", "--ip", HOST, "--port", PORT, "--root-dir", ROOT_DIR,
                       "--server-name", "httpd", "--default-file", "index.html"] if args == [] else [BIN] + args,
                         stdout=out_f, stderr=sp.PIPE, bufsize=0)
    time.sleep(.2)
    return process

def teardown(process):
    process.terminate()
    process.wait()

def test_basic():
    process = setup("/dev/null")
    response = requests.head(f"http://{HOST}:{PORT}/index.html")
    try:
        assert response.status_code == http.HTTPStatus.OK
        assert response._content == http.client.NO_CONTENT
    finally:
        teardown(process)

def test_not_a_real_file():
    process = setup("/dev/null")
    response = requests.head(f"http://{HOST}:{PORT}/not_a_real_file.html")
    try:
        assert response.status_code == http.HTTPStatus.NOT_FOUND
        assert response._content == http.client.NO_CONTENT
    finally:
        teardown(process)

def test_good_content_length():
    process = setup("/dev/null")
    response = requests.head(f"http://{HOST}:{PORT}/index.html")
    try:
        assert response.status_code == http.HTTPStatus.OK
        assert int(response.headers['Content-Length']) == len(response.content)
        assert response._content == http.client.NO_CONTENT
    finally:
        teardown(process)

def test_wrong_method():
    process = setup("/dev/null")
    response = requests.put(f"http://{HOST}:{PORT}/index.html")
    try:
        assert response.status_code == http.HTTPStatus.NOT_IMPLEMENTED
        assert response._content == b'Method Not Implemented\n'
    finally:
        teardown(process)

def test_wrong_version():
    process = setup("/dev/null")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, int(PORT)))
    s.sendall(b"HEAD /index.html HTTP/1.0\r\nHost: " + HOST.encode() + b"\r\n\r\n")
    data = s.recv(1024)
    s.close()
    try:
        assert b"HTTP/1.0 505 HTTP Version Not Supported" in data
    finally:
        teardown(process)

def test_wrong_header_host_twice():
    process = setup("/dev/null")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, int(PORT)))
    s.sendall(f"HEAD /index.html HTTP/1.1\r\nHost: {HOST}\r\nHost: abc\r\n\r\n")
    data = s.recv(1024)
    s.close()
    try:
        assert b"HTTP/1.1 400 Bad Request" in data
    finally:
        teardown(process)

def test_wrong_header_no_host():
    process = setup("/dev/null")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, int(PORT)))
    s.sendall(f"HEAD /index.html HTTP/1.1\r\\r\n")
    data = s.recv(1024)
    s.close()
    try:
        assert b"HTTP/1.1 400 Bad Request" in data
    finally:
        teardown(process)

def test_wrong_header_invalid_char():
    process = setup("/dev/null")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, int(PORT)))
    s.sendall("HEAD /index.html HTTP/1.1\r\nHost: " + HOST.encode() + "\r\nX-Wron£g-Header: oops\r\n\r\n")
    data = s.recv(1024)
    s.close()
    try:
        assert b"HTTP/1.1 400 Bad Request" in data
    finally:
        teardown(process)

def test_log_receive_respond():
    process = setup("/tmp/log")
    _ = requests.head(f"http://{HOST}:{PORT}/index.html")
    
    with open("/tmp/log", "r") as f:
        log_content = f.read()
        try:
            assert f"received HEAD on '/index.html' from 127.0.0.1" in log_content
            assert f"responding with 200 to 127.0.0.1 for HEAD on '/index.html'" in log_content
        finally:
            teardown(process)
