Attach ���ƶ�����
operator= ����

���ֹ��캯������ Attach����


���������������ĳ�����һ�����νṹ
һ�� wavefront �п����ж�� mesh�����ص�ʱ���ֱ�Ӵ������ objects

���͸� Renderer �ĳ�����һ������ƽ���Ľṹ

Scene��
	multiple reflection probe

	multiple lights
		transform

	multiple Objects
		transform
			translation
			rotation
			scaling ���������������ų̶Ȳ�ͬ�� scaling��ע�ⷨ�߽�����

		one Mesh
			Vertices
				position
				normal
				tangent
				albedo
				uv_coordinates (may be multiple...)
			Indices

		one Material (�Զ�����ʣ�)
			shader
				vertex shader
				pixel shader
			data
				albedo			(value or texture)
				metallic		(value or texture)
				roughness		(value or texture)
				transmissive	(value or texture)
			

�ڶ��֡�ϸ��õ����ݳ�Ϊ RenderingResource������ ������Ϣ���������±���Ϣ��shader������

Renderer �ṩ register �� unregistered ����
Mesh��Shader��Texture ���߷ֿ�

Material �� Mesh Ӧ�ö��ǿ��Լ̳еġ���Material Ӧ������ָ���� Mesh ��Ҫ�����Ƿ�֧��ĳЩ���ԡ�����
Material<Mesh> ���ˡ���

Renderable (i.e. objects) ����洢ָ��

�߼��� Renderer �� create balabala ���صĶ��� Renderer ��ת�нṹ�壬�ⲿ����Ҫ������ʲô~
���Դ��ݲ�����ʱ������ balabala ���� shared_ptr ��������
���� Renderer ������Ϊ������ʱ��Ӧ��ʹ�� reference��