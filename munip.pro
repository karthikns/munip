TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = sub_core sub_app sub_tests

sub_core.subdir = core

sub_app.subdir = app
sub_app.depends = sub_core

sub_tests.subdir = tests
sub_tests.depends = sub_core
