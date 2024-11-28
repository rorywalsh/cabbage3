# Cabbage3

This repository contains the **Cabbage3** project. 

> NOTE: Cabbage3 is still very much under development! The source code presented here is experimental and may undergo significant changes. Features are not final, and stability or performance may vary. Use at your own discretion, and expect frequent updates and potential breaking changes.


### Build Instructions

The github repo has a CI build that will produce binaries from teh most up to date source. If you need to build yourself you can do so by running the following cmake command

```bash
cmake -S . -B build -DCABBAGE_BUILD_TARGET=CabbageApp
```

Valid targets are:
* `CabbageApp` : The VSCode service app. This application is ran each time you run an instrument in vscode.
* `CabbageStandaloneApp` : A standalone plugin application
* `CabbageVST3Effect` : The VST3 plugin effect target
* `CabbageVST3Synth` : The VST3 plugin synth target
* `CabbageAUv2Effect` : The AUv2 plugin effect target
* `CabbageAUv2Synth` : The AUv2 plugin synth target

### Getting started with Cabbage3

Temporary docs for Cabbage 3 are available [here](https://rorywalsh.github.io/cabbage3website/docs/intro). These docs will be merged in to the main Cabbage website when the first release is made public. 

