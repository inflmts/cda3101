#!/bin/sh

exec ssh -p 3101 -T root@localhost < pa2.s \
  'sed "s/^#!//" > pa2.s && sed "s/^#@//;t;d" pa2.s | sh'
