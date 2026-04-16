## TCP Server (C++)

Simple TCP / HTTP messenger server written in C++ using low-level Linux sockets.

![Screenshot](Screenshot.png)

## 🚀 Features

• Two servers in one program.

• HTTP server on port `8080`.

• Chat server on port `8081`.

• Login only by username.

• Chat commands:
`/help`
`/ip`
`/hello`
`/echo`
`/html`
`/chat <username>`
`/back`
`/sent_messages`
`/session_messages`
`/exit`

• HTTP login page.

• HTTP menu page after successful login.

• HTTP chats page.

• HTTP personal dialog page:
`/chat/username`

• HTTP routes:
`/`
`/menu`
`/hello`
`/about`
`/chat`
`/logout`

• Web messages update without full page reload.

• User cannot login twice at the same time.

• 404 page for unknown routes.


## ⚙️ Requirements

• Linux / macOS.

• g++ compiler.

• make.


## 💡 Build and Run

Go to `src`:

```bash
cd src
```

Build:

```bash
make
```

Run:

```bash
make run
```

## 📁 Structure

```text
src/
├── chat/
├── http/
├── shared/
├── storage/
└── web/
    ├── pages/
    └── assets/
```


## 🌐 HTTP

Open in browser:

```text
http://127.0.0.1:8080/
```

Main pages:

```text
/
/menu
/chat
/chat/username
```

## 💬 Chat

Connect from terminal:

```bash
nc 127.0.0.1 8081
```

Then enter only your username.
