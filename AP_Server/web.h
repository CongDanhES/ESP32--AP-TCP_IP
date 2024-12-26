const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ĐIỀU KHIỂN TỐC ĐỘ XOAY</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .button {display: inline-block; padding: 10px 20px; font-size: 20px; cursor: pointer; text-align: center; text-decoration: none; outline: none; color: #fff; border: none; border-radius: 15px; box-shadow: 0 9px #999; margin: 10px;}
    .button-increase {background-color: #4CAF50;}
    .button-increase:hover {background-color: #3e8e41;}
    .button-increase:active {background-color: #3e8e41; box-shadow: 0 5px #666; transform: translateY(4px);}
    .button-decrease {background-color: #f44336;}
    .button-decrease:hover {background-color: #d32f2f;}
    .button-decrease:active {background-color: #d32f2f; box-shadow: 0 5px #666; transform: translateY(4px);}
  </style>
</head>
<body>
  <h2>ĐIỀU KHIỂN TỐC ĐỘ XOAY</h2>
  %BUTTONPLACEHOLDER%
<script>
function sendAction(fan, action) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?fan=" + fan + "&action=" + action, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";