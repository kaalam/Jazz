## Jazz uplifts

Jazz uplifts are c++ extensions to the Jazz server that are compiled with it becoming part of the binary.

Jazz uplifts do not make part of core Jazz (https://github.com/kaalam/Jazz) and may have different licenses.

In the main Jazz project the `uplifts/` folder is .gitignore-ed except for this README file.

Normally, the `uplifts/` folder will be a git repository of some other project.

If you have sources in your `uplifts/` folder, you will have to run `./config.sh` each time you create/delete files or their includes
are modified.

### How config.sh expects the structure to be

`./config.sh` expects the following folder structure (without more subfolders, one class per category).
```txt
    uplifts/
    ├── README.md    // This file
    ├── bebop/       // <name>.h, <name>.cpp with a class inherited from Bebop or no `bebop` folder.
    ├── models_api/  // <name>.h, <name>.cpp with a class inherited from ModelsAPI or no `models_api` folder.
    └── api/         // <name>.h, <name>.cpp with a class inherited from API or no `api` folder.
```

1. The "magic" works because `./config.sh` will include the compilation paths and create two `src/uplifted/uplifted_instances.h`,
`src/uplifted/uplifted_instances.cpp` files with the names of the uplifted classes that will replace their parent classes in the server.

2. The model/* project can have as many files as necessary as long as their imports are explicit. You have to define a service that
inherits ModelAPI to serve the models, but also, another instance that inherits Models and the models themselves which inherit Model.

3. To simplify uplift development, your project wil typically live in a separate repository and possibly with different folder names
and you can use a script like `enable_uplifts.sh` to maintain the symbolic links to the expected structure.

### Uplifts Reference

  * https://kaalam.github.io/jazz_reference/using_extend.html
