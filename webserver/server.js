var express = require("express");
const PORT=80;
var app = express();
app.use(express.static("."));
app.listen(PORT, function() {
  console.log("Express app started on port " + PORT);
});
