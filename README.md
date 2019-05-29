# Ball game server and client


## Building

On Ubuntu, simply install the build-essential package.

```sh
sudo apt-get install build-essential
```

Then run `make` in this folder, the build should finish in no time.

## Deploy Server on Docker

If you don't want to run the server on your machine, you can use Docker instead.

Make sure you already have Docker installed and configured correctly, in this folder run

```sh
docker build -t ball-server-lite .
```

Wait a while for all the necessary images to download and you're good to go. Once everything is done, we start the server with

```sh
docker run -a stdout --rm --net=host ball-server-lite
```

Then we can connect to it normally.

## Running locally and start test clients.

After you finish building, run `./sv` to start the server.

Give permission to the client start script

```sh
chmod +x startclient.sh
```

Then start the test clients by running `startclient.sh`


## Modify client script to add more client


Currently the number of client is set at 4, however we can increase the number of client up to 10 by modifying the start script.


Currently it looks like this:

```sh
./client 1 & ./client 2 & ./client 3 & ./client 4 wait
```

To add another client, just add `& ./client "id"` in right in front of wait

For example if you want to add a fifth client, it should look like this

```sh
./client 1 & ./client 2 & ./client 3 & ./client 4 & ./client 5 wait
```