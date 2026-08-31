#!/bin/sh

if ! which -s git; then
	echo "git was not found" >&2
	exit 1
fi

git pull --tags --rebase git://git.claws-mail.org/claws.git

