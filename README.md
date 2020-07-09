# Volumetric-Clouds
Volumetric Clouds for X-Plane 11

### Linux Build

* install dependencies
```
sudo apt install libglew-dev libpng-dev
```

* checkout the code
```
git clone https://github.com/mSparks43/Volumetric-Clouds.git
cd Volumetric-Clouds
git submodule init
git submodule update
```

* download the x-plane sdk and place inside as "SDK" directory
```
https://developer.x-plane.com/sdk/plugin-sdk-downloads/
```

* Build the plugin
```
make
```

* Copy the files to your X-Plane `plugins` directory
```
cp -Rf Volumetric_Clouds ~/X-Plane_11/Resources/plugins/
```
