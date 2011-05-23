PROJECT_NAME = Racesow
PROJECT_VERSION_MAJOR = 0
PROJECT_VERSION_MINOR = 6
PROJECT_VERSION_PATCH = 1
PROJECT_VERSION_NUMBER = $(PROJECT_VERSION_MAJOR).$(PROJECT_VERSION_MINOR).$(PROJECT_VERSION_PATCH)
SIMPLE_VERSION_NUMBER = $(PROJECT_VERSION_MAJOR)$(PROJECT_VERSION_MINOR)$(PROJECT_VERSION_PATCH)
PROJECT_VERSION_STRING = $(PROJECT_NAME) $(PROJECT_VERSION_NUMBER)

DOC_DIRECTORY = doc
DOXYGEN_FILE = api.dox
RACESOW_DIRECTORY = racesow
GAMETYPE_DIRECTORY = racesow/progs/gametypes/racesow/
SOURCE_DIRECTORY = source

.PHONY: doc

doc: 
	cd doc; doxygen $(DOXYGEN_FILE)
	
version:
	find $(GAMETYPE_DIRECTORY) -name "*.as" \
	-exec sed -i -e 's/@version.*/@version $(PROJECT_VERSION_NUMBER)/' {} \;
	sed -i -e 's/setVersion(.*)/setVersion( "$(PROJECT_VERSION_NUMBER)" )/' \
	$(GAMETYPE_DIRECTORY)/main.as
	sed -i -e 's/\(PROJECT_NUMBER *=\).*/\1 $(PROJECT_VERSION_NUMBER)/' \
	$(DOC_DIRECTORY)/$(DOXYGEN_FILE)
	
gametype:
	cd $(RACESOW_DIRECTORY); zip -r racesow_gametype$(SIMPLE_VERSION_NUMBER)pure progs cfgs
	mv $(RACESOW_DIRECTORY)/racesow_gametype$(SIMPLE_VERSION_NUMBER)pure.zip racesow_gametype$(SIMPLE_VERSION_NUMBER)pure.pk3

data:
	cd $(RACESOW_DIRECTORY); zip -r racesow_data$(SIMPLE_VERSION_NUMBER)pure gfx huds
	mv $(RACESOW_DIRECTORY)/racesow_data$(SIMPLE_VERSION_NUMBER)pure.zip racesow_data$(SIMPLE_VERSION_NUMBER)pure.pk3

install: gametype data
	mv  racesow_data$(SIMPLE_VERSION_NUMBER)pure.pk3 racesow_gametype$(SIMPLE_VERSION_NUMBER)pure.pk3 $(SOURCE_DIRECTORY)/release/racesow/

clean:
	rm *pk3