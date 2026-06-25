TEMPLATE = subdirs

# Build order: mcpcore first, then mcpclient/mcpserver (can be parallel),
# then examples, finally tests
SUBDIRS += \
    mcpcore \
    mcpclient \
    mcpserver \
    examples \
    tests

# Ensure dependency order
mcpclient.depends = mcpcore
mcpserver.depends = mcpcore
examples.depends = mcpclient mcpserver
tests.depends = mcpcore mcpclient mcpserver

