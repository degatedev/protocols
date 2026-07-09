# DeGate Protocol v0.1.0

## Build

### MacOS
If you are using a Mac, you will need to (re）install the commandline tool:

```
	sudo rm -rf /Library/Developer/CommandLineTools
	xcode-select --install

```

Then you may also need to install "lgmpxx":

- Download the source from https://gmplib.org/download/gmp/gmp-6.2.0.tar.lz
- unzip it using (lzip - `brew install lzip`) :`tar -xf gmp-6.2.0.tar.lz`
- install it:

```
	./configure --prefix=/usr/local --enable-cxx
	make
	make check
	sudo make install
```

### Build steps
Use the following build dependencies:
- node v14.15.5 (`nvm install 14.15.5`)
- python 3.10	(`pyenv local 3.10`)

Execute the following in the `protocols` directory:
- `rm packages/loopring_v3/contracts/thirdparty/timelock/ -r`
> The Timelock was directly copied from Compound, so its version is ^0.8.0 which is indeed incompatible with degate contracts. To compile the entire  of contracts to understand the degate protocol, you can first remove the Timelock. It can be deployed separately afterwards. [*](https://discord.com/channels/787092485969150012/1176356057505865758/1176443266154123264)
- `git submodule update --init --recursive`
- `npm install`
- `npm run build` or `npm run compile` or `npm run watch`.

### Circuits

The circuit tests can be run with `npm run testc`. A single test can be run with `npm run test-circuits <test_name>`.

## Run Unit Tests

- please make sure you run `npm run build` for the first time.
- run `npm run ganache` from project's root directory in terminal.
- run `npm run test` from project's root directory in another terminal window.
- run single test: `npm run test -- transpiled/test/xxx.js`
- print info logs in tests: `npm run test -- -i`
- print more detailed debug logs in tests: `npm run test -- -x`

Running all tests takes around 1 hour on a modern PC with a CPU with 4 cores. Creating proofs is computationaly heavy and takes time even when multi-threading is used. Run individual tests when you can.

Verifier/Prover keys are cached in the `keys` folder. When running `make` these keys are automatically deleted so they cannot be outdated.

## Contract Deployment

- development network: `npm run migrate-dev`
- ropsten network: `npm run migrate-ropsten`
- main network: `npm run migrate`

If you have installed truffle globally, you can run:

`npm run transpile && truffle migrate --network <network> --reset --dry-run`

Replace network with `development`, `live`, `ropsten`, `koven`, etc.