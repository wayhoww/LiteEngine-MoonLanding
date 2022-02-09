Attach 是移动语义
operator= 不是

各种构造函数都用 Attach。。


场景管理器看到的场景是一个树形结构
一个 wavefront 中可能有多个 mesh，加载的时候就直接处理成子 objects

发送给 Renderer 的场景是一个“拍平”的结构

Scene：
	multiple reflection probe

	multiple lights
		transform

	multiple Objects
		transform
			translation
			rotation
			scaling （包括各方向缩放程度不同的 scaling，注意法线矫正）

		one Mesh
			Vertices
				position
				normal
				tangent
				albedo
				uv_coordinates (may be multiple...)
			Indices

		one Material (自定义材质？)
			shader
				vertex shader
				pixel shader
			data
				albedo			(value or texture)
				metallic		(value or texture)
				roughness		(value or texture)
				transmissive	(value or texture)
			

在多个帧上复用的数据称为 RenderingResource，包括 顶点信息，三角形下标信息，shader，纹理

Renderer 提供 register 和 unregistered 方法
Mesh、Shader、Texture 三者分开

Material 和 Mesh 应该都是可以继承的。。Material 应当可以指定对 Mesh 的要求（如是否支持某些属性。。）
Material<Mesh> 好了。。

Renderable (i.e. objects) 里面存储指针

逻辑上 Renderer 的 create balabala 返回的都是 Renderer 的转有结构体，外部不需要管他是什么~
所以传递参数的时候，这种 balabala 都用 shared_ptr 当作参数
但是 Renderer 本身作为参数的时候应该使用 reference。