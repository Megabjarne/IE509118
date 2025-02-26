#!/bin/bash

gzip -kc ../ui/index.html > /tmp/index.html.gz
xxd -i /tmp/index.html.gz index.h
sed -i "s/_tmp_index_html_gz/index_html_gz/" index.h
mv index.h ../SensorFish/
rm /tmp/index.html.gz
