#!/bin/bash

dotnet publish
sudo cp ./bin/Release/net8.0/linux-x64/publish/darch /usr/local/bin/darch