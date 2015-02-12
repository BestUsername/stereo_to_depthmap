# stereo_to_depthmap
C++ program to compute a depth map video from stereo video files.

Currently supports both command line processing as well as a gui option with live preview.
Can process entire videos or subsections (different scenes may require different settings).

Still a little rough around the edges.

![Screenshot](https://github.com/BestUsername/stereo_to_depthmap/blob/master/extras/screenshots/depthmap_snake.png)
For explanation of controls, look at StereoSGBM constructor:
http://docs.opencv.org/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html#stereosgbm-stereosgbm
