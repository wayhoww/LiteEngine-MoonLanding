# Moon Landing
��½����С��Ϸ


## Build

���û�����Windows 10/11, Visual Studio 2019 ����߰汾

������NuGet �� DirectXTeX�������������£�Visual Studio 
����������

�������裺
1. �Ҽ����� MoonLanding ��Ŀ����������Ϊ������Ŀ
2. ���Ҫ�� Visual Studio ��ֱ����������Ը���Ŀ����ô��
   Ҫ�Ҽ����� MoonLanding ��Ŀ���� ���� - ���� - ����Ŀ
   ¼����Ϊ "$(TargetDir)"����Ϊ Visual Studio ���Ὣ
   �������ñ��浽 VC ��Ŀ�ļ���������Ҫ�ֶ����ø��
3. ���� / ���� / ����


## Run

������ɺ���Ŀ�����������Դ�ļ����� cso �ļ���3D ģ����
�������Ϳ�ִ�г���λ��ͬһĿ¼�¡��Ը�Ŀ¼Ϊ����Ŀ¼��������
���ɡ�


## Dependencies

0. Windows API
1. NuGet �� DirectXTeX��Visual Studio ���Զ���װ
2. TinyGLTF�����ڼ��� glTF 3D ģ���ļ�����һ�� header-only
   �Ŀ�


## Structures

* LiteEngine: "��������"���򵥷�װ�� DirectX �� API
* MoonLanding: ��½����С��Ϸ

## LiteEngine

### Modeling
ʹ�� Blender ��ģ�������ⲿģ�ͣ��������� glTF 2.0 ��ʽ��
�ڵ������棬��Ҫ��ѡ Normals��Tangents����Ҫȡ����ѡ +Y Up
(������������������)�����Թ�ѡ Punctual Lights������⡢
��⡢�۹�ƣ���Cameras���������

���һ�� Mesh û����ʽ���� TANGENT������û���κ� UV����ô 
Blender ����������ᵼ�� TANGENT����������£������� Blender 
���ֶ���� UV Map ���ٵ�����

��ȱ�� NORMAL ���� TANGENT ��ʱ��Ϊ�˼���ȷ�����
LiteEngine ��ֱ�ӱ���

### glTF Model Loader Feature
�ᵼ��:
1. Mesh
2. �ƹ�
3. ���ʺ���ͼ
4. ���

���ᵼ�룺
1. ����

��Ҫ glTF �ļ��д��ڣ�
1. ����
2. ������

��֧�֣�
1. �������ε���Ϣ
2. ��ϡ�軺�����ж�ȡ����



## Reference
��Ҫ�ο�����
* [Direct3D 11 Graphics](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
* [Blender Reference Manual](https://docs.blender.org/manual/en/latest/)
* [glTF 2.0 Specification](https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html)
* [glTF `KHR_lights_punctual` Extension �ĵ�](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md)
