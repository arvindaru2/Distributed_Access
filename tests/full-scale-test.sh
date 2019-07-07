#!/bin/sh

../grep -c images | sort > tmp-images.out
diff tmp-images.out full-scale-images.out
rm tmp-images.out

