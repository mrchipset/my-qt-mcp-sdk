TEMPLATE = subdirs

# Test subprojects (ordered by dependency)
SUBDIRS += \
    test_mcptypes \
    test_jsonrpc \
    test_transport \
    test_server \
    test_client

# Ensure dependency order for test execution compilation
test_jsonrpc.depends = test_mcptypes
test_transport.depends = test_mcptypes
test_server.depends = test_mcptypes test_jsonrpc test_transport
test_client.depends = test_mcptypes test_jsonrpc test_transport
