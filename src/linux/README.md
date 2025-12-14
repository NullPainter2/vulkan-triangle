# I used .spv for different gpu

# VK_LAYER_KHRONOS_validation is not available?

Add vulkan SDK source
```
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt update
sudo apt install vulkan-sdk
```

Goal is to install this
```
sudo apt update
sudo apt install vulkan-validationlayers-dev spirv-tools vulkan-sdk
```

# present que index not found

X11 Window wasn't created and valid.
