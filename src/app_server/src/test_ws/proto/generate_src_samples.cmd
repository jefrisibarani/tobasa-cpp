@echo off

:: Before generating for Javascript
:: install globally
:: npm install -g protobufjs
:: npm install -g protobufjs-cli

SET PATH=%PATH%;d:\dev\protobuf_x64-windows-static-vs2017\tools\protobuf;
SET TBS_ROOT=d:\projects\tobasa_cxx

:: For C++
protoc --cpp_out=%TBS_ROOT%\src\app_server\src\test_ws\proto chat_types_samples.proto

:: For browser
pbjs -t static-module -w commonjs -o %TBS_ROOT%\src\app_server\wwwroot\js\chat_types_samples.js chat_types_samples.proto

:: After chat_types_samples.js generated,
:: on the 4th line,   var $protobuf = require("protobufjs/minimal");
:: replace with:      var $protobuf = window.protobuf;
:: on the last line,  module.exports = $root;
:: replace with:      //module.exports = $root;


:: For non browser 
:: pbjs -t static-module -w es6 -o src/chat_types_samples.js proto/chat_types_samples.proto