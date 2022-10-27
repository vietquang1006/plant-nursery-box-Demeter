const express = require("express");
const app = express();
const http = require("http");
const server = http.createServer(app);
const { Server } = require("socket.io");
const io = new Server(server, {
  allowEIO3: true,
  cors: {
    origin: "*",
    methods: ["GET", "POST"],
    transports: ["websocket", "polling"],
    credentials: true,
  },
});

app.use(express.static("public"));

app.get("/", (req, res) => {
  res.sendFile(__dirname + "/index.html");
});

io.on("connection", (socket) => {
  console.info(
    "[" + socket.id + "] new connection",
    socket.request.connection.remoteAddress
  );

  //xu ly cho esp client
  socket.on("message", (data) => {
    console.log(data);
    socket.broadcast.emit("message", data);
  });

  //xu ly cho web client, esp client
  socket.on("UPDATE", (data) => {
    console.log(`update for plant ${data}`);
    socket.broadcast.emit("UPDATE", data);
  });
  socket.on("LED_on", (data) => {
    console.log(`bat den plant ${data}`);
    socket.broadcast.emit("LED_on", data);
  });
  socket.on("LED_off", (data) => {
    console.log(`tat den plant ${data}`);
    socket.broadcast.emit("LED_off", data);
  });
  socket.on("QUAT_on", (data) => {
    console.log(`bat quat plant ${data}`);
    socket.broadcast.emit("QUAT_on", data);
  });
  socket.on("QUAT_off", (data) => {
    console.log(`tat quat plant ${data}`);
    socket.broadcast.emit("QUAT_off", data);
  });
  socket.on("PHUNSUONG_on", (data) => {
    console.log(`bat phun suong plant ${data}`);
    socket.broadcast.emit("PHUNSUONG_on", data);
  });
  socket.on("PHUNSUONG_off", (data) => {
    console.log(`tat phun suong plant ${data}`);
    socket.broadcast.emit("PHUNSUONG_off", data);
  });

  //xu ly chung
  socket.on("reconnect", function () {
    console.warn("[" + socket.id + "] reconnect.");
  });
  socket.on("disconnect", () => {
    console.error("[" + socket.id + "] disconnect.");
  });
  socket.on("connect_error", (err) => {
    console.error(err.stack);
  });

  //test nodemcu websocket code mẫu
  socket.on("event_name", (data) => {
    console.log(data);
  });
});

server.listen(3600, () => {
  console.log("server is listening on port 3600");
});
