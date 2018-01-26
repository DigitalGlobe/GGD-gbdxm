# gbdxm
A packaging tool for creating models readable by DeepCore based applications and tools.

For more information, please visit: https://digitalglobe.github.io/DeepCore/ 

Download the x64 Linux binary file here: https://s3.amazonaws.com/deepcore/current/gbdxm 


## Model Support

The characteristics of the supported models are described in depth in the
[Model Reference](doc/modelreference.md).  At this time, models are supported in
three categories:
 
 - Classification: The neural network performs classification on each image
 that is provided.
 - Segmentation: The neural network performs per-pixel classification which may
 be vectorized into shapes.
 - Detection: The output of the neural network immediately interpretable in
 vector space.  For instance, a model may output bounding boxes with confidences.
 
The particulars of each models what is supported vary by framework as specified
in the model reference.