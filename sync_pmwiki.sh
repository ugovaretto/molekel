#!/bin/bash
rm -f ~/public_html/admin/staging/wiki/wiki.d/.flock
rsync -a --delete --exclude 'Site.SiteHeader' ~/public_html/admin/staging/wiki/ ~/public_html/wiki/
