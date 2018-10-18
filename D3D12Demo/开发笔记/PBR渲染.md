## BRDF Material

纹理加载库 https://github.com/Microsoft/DirectXTex/tree/master/DirectXTex

https://github.com/Microsoft/DirectXTex/wiki/TGA-I-O-Functions

构建库的时候，选择Win10版本，不然不支持DX12



## Normal Map 法线映射

在使用公式来计算时；

- 按照每个三角形来计算Tangent 和BiNormal，此时计算出来的值并不是单位矢量，只有这样才能通过 TBN 将切线空间的值转换对了；
- 然后再在每个顶点上进行平均；
- 只计算 N 和 T ，到了ps阶段再计算 B；

- 注意手相性；
- https://www.cnblogs.com/sddai/p/5928101.html











