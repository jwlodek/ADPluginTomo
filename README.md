# ADPluginTomo

An `areaDetector` plugin designed to stream tomographic detector data into a real time reconstruction client during acquisiton.

### Installation

To install ADPluginTomo, clone it into your `areaDetector` directory, enter it and run:

```
make
```

Note that `ADSupport`, `ADCore`, and any external dependencies must be built/installed first. 

Next, open `$(AREA_DETECTOR)/configure/RELEASE_PRODS.local`, and add:

```
ADPLUGINTOMO=$(AREA_DETECTOR)/ADPluginTomo
```

Then, add the following to `$(AREA_DETECTOR)/ADCore/ADApp/commonDriverMakefile`

```
ifdef ADPLUGINTOMO
  $(DBD_NAME)_DBD += NDPluginTomo.dbd
  PROD_LIBS += NDPluginTomo
  # Add any external library dependancy links here
  # PROD_SYS_LIBS += ... For system libraries
  # PROD_LIBS += ... For libraries built as part of the plugin build process
endif
```

Next, enter the target `areaDetector` driver directory and rebuild it with `make`.

Finally, in either your IOC `st.cmd` startup file, or in `$(AREA_DETECTOR)/ADCore/iocBoot/commonPlugins.cmd` initialize the plugin for startup:

```
NDTomoConfigure("TOMO1", $(QSIZE), 0, "$(PORT)", 0, 0, 0, 0, 0, $(MAX_THREADS=5))
dbLoadRecords("$(ADPLUGINTOMO/db/NDPluginTomo.template", "P=$(PREFIX), R=Tomo1:, PORT=TOMO1, ADDR=0, TIMEOUT=1, NDARRAY_PORT=$(PORT), NAME=TOMO1, NCHANS=$(XSIZE)")
set_requestfile_path("$(ADPLUGINTOMO)/db")
```

### Credits

This plugin was built in part with the help of [ADPluginTemplate](https://github.com/jwlodek/ADPluginTemplate).
