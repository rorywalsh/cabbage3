# Cabbage3

This repository contains the **Cabbage3** project.



### Build Instructions

```bash
//todo
```


### Running the VSCode Extension

In order to use the Cabbage service app you'll need to build and run the `vscabbage` extension:

```bash
git clone https://github.com/rorywalsh/vscabbage
cd vscabbage
npm install
```

Open the `vscabbage` directory with Visual Studio Code and hit f5 to start debugging. When the extension launches, go to the extension settings and updates the path to `CabbageApp`, and the path to the Cabbage javascript source. They appear as:

**Cabbage: Path To Cabbage Executable** 

and 

**Cabbage: Path To JS Source**

The folder you need to point to for the JS source is `vscabbage/src/`. Once these paths are set up can try opening and running some of the examples in the `cabbage3/test/widgets` folder, the slider examples should all work fine for testing. `Ctrl+S` will run the new instrument. `Ctrl+E` will stop the instrument and bring you to edit mode. 


