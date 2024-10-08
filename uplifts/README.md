## Jazz uplifts

Jazz uplifts are c++ extensions to the Jazz server that are compiled with it becoming part of the binary.

Jazz uplifts do not make part of core Jazz (https://github.com/kaalam/Jazz) and may have different licenses.

In the main Jazz project the `uplifts/` folder is .gitignore-ed except for this README file.

Normally, the `uplifts/` folder will be a git repository of some other project.

If you have sources in your `uplifts/` folder, you will have to run `./config.sh` each time you create/delete files or their includes
are modified.

### How config.sh expects the structure to be

`./config.sh` expects the following folder structure (without more subfolders, one class per category).

    uplifts/
    ├── README.md  // This file
    ├── api/       // <name>.h, <name>.cpp with a class inherited from Api      (in jazz_main/api.h)       or no `api` folder.
    └── model/     // <name>.h, <name>.cpp with a class inherited from ModelAPI (in jazz_model/model.h)    or no `model` folder.

1. The "magic" works because `./config.sh` will include the compilation paths and create two `src/uplifted/uplifted_instances.h`,
`src/uplifted/uplifted_instances.cpp` files with the names of the uplifted classes that will replace their parent classes in the server.

2. The model/* project can have as many files as necessary as long as their imports are explicit. You have to define a service that
inherits ModelAPI to serve the models, but also, another instance that inherits Models and the models themselves which inherit Model.
