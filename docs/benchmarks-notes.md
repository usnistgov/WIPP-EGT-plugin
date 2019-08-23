valgrind --tool=massif ./commandLineCli -i "/home/gerardin/Documents/pyramidBuilding/outputs/poster.tif" 
-o "/home/gerardin/CLionProjects/newEgt/outputs2/" -d "8U" --level "2" --minhole "50" --maxhole "inf"
 --minobject "250" -x "0" -e "loader=2;tile=10;intensitylevel=2;sample=20;exp=5"

==8558==

=> between 1 and 2GB

----

valgrind --tool=massif ./commandLineCli -i "/home/gerardin/Documents/pyramidBuilding/outputs/poster.tif"
 -o "/home/gerardin/CLionProjects/newEgt/outputs2/" -d "8U" --level "0" --minhole "50" --maxhole "inf"
  --minobject "250" -x "0" -e "loader=2;tile=30;intensitylevel=0;sample=20;exp=5"

Segmentation Fault