TEMPLATE = subdirs

process.depends = core
webengine.depends = core
webenginewidgets.depends = core
webengine_plugin.subdir = webengine/plugin
webengine_plugin.target = sub-webengine-plugin
webengine_plugin.depends = webengine
webengine_experimental_plugin.subdir = webengine/plugin/experimental
webengine_experimental_plugin.target = sub-webengine-experimental-plugin
webengine_experimental_plugin.depends = webengine


SUBDIRS += core \
           process \
           webengine \
           webengine_plugin \
           webengine_experimental_plugin

# FIXME: We probably want a bunch of functions and config options to tweak what to build/ship or not
WEBENGINE_CONFIG = use_default_ui_delegates
contains(WEBENGINE_CONFIG, use_default_ui_delegates): SUBDIRS += lib/quick/ui

qtHaveModule(widgets) {
    SUBDIRS += webenginewidgets
}
