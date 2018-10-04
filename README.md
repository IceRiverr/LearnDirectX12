# LearnDirectX12

To learn new graphics api

学习新一代图形API

主要参考 Introduction to 3D Game Programming with DirectX12来进行学习

[书的源码](http://www.d3dcoder.net/d3d12.htm)

一定要实现 CPU端的多线程和 GPU的多线程同步

[Direct3D 12 Programming Guide](https://docs.microsoft.com/zh-cn/windows/desktop/direct3d12/directx-12-programming-guide)



## Chapter 4

### 纹理格式 与 纹理的压缩

### 4.1.4 swap chain

Present(): Swapping the front and back buffers

### 4.1.5 Depth buffering or z-buffering

### 4.1.6 Resources and Descriptors

Resource:  Buffers, Textures

Descriptors: Hardware view of object: address + metadata

是对资源的一个轻量级表示，用来告诉这个资源对应的信息，来绑定的pipeline中

引入Descriptor的原因：

- Resource在显存中是一块普通的内存，资源本身不知道自己到底代表什么类型；很那像C++中通过类来明确确定这一块内存块中存的是某个确定的数据；
- 资源一般化，可以让其绑定到pipeline 的不同阶段；无论是SRV还是RTV
- 有时我们只想bind一块资源的一部分区域

Descriptor 应该在初始化时创建，因为需要类型checking and validation

尽量创建确定类型的资源，这样runtime会优化access；

除非需要Typeless 的资源来提供灵活性，才需要创建typless 的 resource

### 4.1.7 Multisampling Theory

- SuperSampling 超采样，使用屏幕空间4x的大小来渲染，然后downsample；

- multisampling D3D提供的技术，也是用4x大小来渲染，只是1个像素的4个子像素只计算一次color和depth，ps对于每个像素只执行一遍，然后通过检测子像素是否在三角形内，如果在内，则给其父像素的值，不在内则使用原来的值，然后再downsample时进行平均；

  可以指定不同的pattern

### 4.1.8 Multisampling in Direct3D

一般的采样数是4x，采样质量可以自己设置

如果不需要则 Sample_Count = 1, Sample_Quality = 0

### 4.1.9 Feature Levels

一种Feature Level 会定义其特定的功能集；

不同的GPU可能对最新Feature Level支持不好，所以使用时需要遍历一下；

### 4.1.10 DirectX Graphics Infrastructure

DXGI 的基本思想是，一些图形人物是统一的，可以共用，因此可以抽取出来，左右一个共用的接口；

eg. IDXGISampChain, full-screen mode, graphical system information

IDXGIFactory, IDXGIAdapter（一般代表显卡）, 

[DXGUI Overview](https://docs.microsoft.com/zh-cn/windows/desktop/direct3ddxgi/d3d10-graphics-programming-guide-dxgi)

[DirectX Graphics Infrastructure (DXGI): Best Practices](https://docs.microsoft.com/zh-cn/windows/desktop/direct3darticles/dxgi-best-practices)

Adaptor--Output--Swapchian--window 一一对应

全屏转化，会产生Resize消息，需要注意

IDXGISwapChain::Present1(DXGI_PRESENT_TEST) 可以应对窗口被遮挡的情况；

ID3D11DeviceContext::OMSetRenderTargets() 刚好在write to back buffer 0 之前；



### 4.1.12 Residency 资源驻留的管理

复杂的游戏需要严格管理GPU 显存；

不需要的资源需要赶出内存，单不要频繁swap in and out，最好以Level 或Area来管理

ID3DDevice::MakeResident() 贮存资源

ID3DDevice::Evict() 回收资源

[Residency](https://docs.microsoft.com/zh-cn/windows/desktop/direct3d12/residency)



### 4.2 Cpu/Gpu Interaction

图形编程是在cpu和gpu两个处理器上工作；

从优化的角度来说，目标是使两者尽可能忙碌，并且减少同步，因为同步就代表两者中的一个需要等待另一个处理完；

### 4.2.1 CommandQueue and Command Lists

可以指定多显卡渲染，比如Intel集显和N卡，一起来渲染一个东西；

通过Command Allocator来创建Command List，Command List 记录的渲染命令比保存在了Allocator中了；

一个Command Allocator，可以创建多个CommandLists，但是某个时刻，只能有一个Open_State Command List；

CommandQueue会执行CommandLists中的命令，但是会引用CommandAllocator中的资源，随意在释放是需要注意；

### 4.2.2 Cpu/Gpu Synchronization

Fence，同步点，用来让GPU追上CPU，以便CPU使用GPU产生的数据，或者其他；

4.2.3 Resource Transitions

resource hazard，资源的读写访问错乱

资源可能处于不同的状态，default state-->render target state-->shader resource state

d3dx12.h 微软提供的一个辅助类

4.2.3 Multithreading with Commands

- Command list are not free-threaded;一个线程一个Commandlist

- Command allocators are not free-threaded; 一个线程一个

- Command queue is free-threaded，多个线程可以自由提交command list；

  可以参考Multithreading12 SDK



## Chapter 6 Draw Box

先使用Dx的数学库，以后换掉，使用一个开源的库；

Constant Buffers

其最小大小为256bytes，随意一个ConstantBuffers可以包含多个Buffer，用来打包；在bind时按照SubArea来制定；

CPU:  XMFloat4x4 row-major 行主序，其内存布局为 m00,m01...

GPU:  float4x4 coloum-major 列主序，其内存布局为 m00, m10...

所有在将g_WorldViewProjMatrix从cpu传递到gpu是，需要进行转置

```c++
XMStoreFloat4x4(&m_ConstantBufferData.WorldViewProj, 		XMMatrixTranspose(mWorldViewProj));
memcpy(m_pCbvDataBegin, &m_ConstantBufferData, sizeof(m_ConstantBufferData));
```

[Why is this Transpose() required in my WorldViewProj matrix?](https://stackoverflow.com/questions/32037617/why-is-this-transpose-required-in-my-worldviewproj-matrix)

[Per-Component Math Operations-Matrix Ording](https://docs.microsoft.com/zh-cn/windows/desktop/direct3dhlsl/dx-graphics-hlsl-per-component-math#Matrix_Ordering)



## BRDF Material

纹理加载库 https://github.com/Microsoft/DirectXTex/tree/master/DirectXTex

https://github.com/Microsoft/DirectXTex/wiki/TGA-I-O-Functions

构建库的时候，选择Win10版本，不然不支持DX12











