#!/bin/bash
docker run -v ~/Dropbox/repos/ptpraw:/ptp -p 1320:1320/udp -ti ramcip
