#!/bin/bash

dotnet publish
sudo cp ./bin/Release/net8.0/osx-arm64/publish/darch /usr/local/bin/darch
