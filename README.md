#Delaunay Triangulation
C++ code to triangulate and visualize point cloud data given in .pcd file.

Read More about Delaunay triangulation Here https://en.wikipedia.org/wiki/Delaunay_triangulation

The code reads the points from a file in variables xcoor, ycoor and zcoor.
Assign your own points to xcoor, ycoor and zcoor if not using a .pcd file.

It first projects the 3D points to XY plane and then does the triangulation in the plane.
Finally the lines are made between the corresponding points in 3D.

![ScreenShot](https://cloud.githubusercontent.com/assets/1044135/18031453/755e1b1c-6caf-11e6-822f-b688424f2a97.png)

