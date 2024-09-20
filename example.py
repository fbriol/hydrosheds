import hydrosheds
import matplotlib.pyplot as plt
import numpy

# Download the HydroSHEDS dataset here:
# https://www.hydrosheds.org/hydrosheds-core-downloads
# Change <folder> to the path where you downloaded the dataset
sheds = [
    '<folder>/hydrosheds/af_msk_3s.tif',
    '<folder>/hydrosheds/as_msk_3s.tif',
    '<folder>/hydrosheds/au_msk_3s.tif',
    '<folder>/hydrosheds/eu_msk_3s.tif',
    '<folder>/hydrosheds/na_msk_3s.tif',
    '<folder>/hydrosheds/sa_msk_3s.tif',
    '<folder>/hydrosheds/hyd_ar_msk_15s.tif',
    '<folder>/hydrosheds/hyd_eu_msk_15s.tif',
    '<folder>/hydrosheds/hyd_gr_msk_15s.tif',
    '<folder>/hydrosheds/hyd_si_msk_15s.tif',
]

# Create a dataset object that will be used to query the HydroSHEDS dataset
# Each tile is 256x256 pixels and the cache can store up to 4096 tiles
# The input coordinates are in EPSG:4326 (WGS84). This ESPG code is used
# to convert the query coordinates to the pixel coordinates of the HydroSHEDS
# dataset.
hs = hydrosheds.Dataset(sheds,
                        espg_code=4326,
                        tile_size=256,
                        max_cache_size=4096)

# For this example, we will create a mask of the water bodies on a global grid
# with a resolution of 0.25 degrees. The mask will be True for water and False
# for land.
step = 0.25
lon = numpy.arange(-180, 180, step)
lat = numpy.arange(-90, 90, step)

# Create a meshgrid of the coordinates
mx, my = numpy.meshgrid(lon, lat)

# Query the HydroSHEDS dataset to get the mask of the water bodies. The input
# coordinates are in EPSG:4326 (WGS84) and must be in the same coordinate
# system as the one used to create the dataset object.
#
# The number of threads is set to 0 to use all the available threads. Adjust
# this to your needs.
#
# The input arrays must be one-dimensional and have the same size. The output
# array will have the same shape as the input arrays.
#
# So here we need to flatten the input arrays and reshape the output array to
# get the mask of the water bodies.
mask = hs.is_water(mx.ravel(), my.ravel(), num_threads=0)
mask = mask.reshape(mx.shape)

# Finally, we plot the mask of the water bodies
plt.figure()
plt.imshow(mask, origin='lower', extent=(lon[0], lon[-1], lat[0], lat[-1]))
plt.savefig('mask.png')
