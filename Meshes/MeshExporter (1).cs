using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

public class MeshExporter : MonoBehaviour
{
	[SerializeField]
	bool flipZAxis = false;

	[SerializeField]
	bool legacyAnim = false;

	[SerializeField]
	bool forceMeshBindPose = false;

	[SerializeField]
	string animName = "";

	SkinnedMeshRenderer[] skinnedRenderers;
	MeshRenderer[] renderers;

	struct BoneInfo {	
		public Transform bone;
		public int depth;
    }

	List<BoneInfo> boneRefs = new List<BoneInfo>();

	List<string> addedBones = new List<string>();

	enum  GeometryChunkTypes
	{
		VPositions		= 1 << 0,
		VNormals		= 1 << 1,
		VTangents		= 1 << 2,
		VColors			= 1 << 3,
		VTex0			= 1 << 4,
		VTex1			= 1 << 5,
		VWeightValues	= 1 << 6,
		VWeightIndices	= 1 << 7,
		Indices			= 1 << 8,
		JointNames		= 1 << 9,
		JointParents	= 1 << 10,
		BindPose		= 1 << 11,
		BindPoseInv		= 1 << 12,
		Material		= 1 << 13,
		SubMeshes		= 1 << 14,
		SubMeshNames	= 1 << 15,
	};

	int BoneDepth(Transform b)
    {
		Transform p = b.parent;
		int d = 0;
		while (p)
		{
			p = p.parent;
			d++;
		}
		return d;
	}


	// Start is called before the first frame update
	void Start()
    {
		skinnedRenderers = GetComponentsInChildren<SkinnedMeshRenderer>();
		renderers		 = GetComponentsInChildren<MeshRenderer>();

		foreach (SkinnedMeshRenderer r in skinnedRenderers)
		{
			if (forceMeshBindPose)
			{
				for (int i = 0; i < r.bones.Length; ++i)
				{
					BoneInfo info	= new BoneInfo();
					info.bone		= r.bones[i];
					info.depth		= BoneDepth(info.bone);
					boneRefs.Add(info);
					addedBones.Add(info.bone.name);
				}
			}
			else
			{
				for (int i = 0; i < r.bones.Length; ++i)
				{
					if (!addedBones.Contains(r.bones[i].name))
					{
						BoneInfo info = new BoneInfo();
						info.bone = r.bones[i];
						info.depth = BoneDepth(info.bone);
						boneRefs.Add(info);
						addedBones.Add(info.bone.name);
					}
				}
			}
		}

		foreach(MeshRenderer r in renderers)
        {
			Transform t = r.transform;
			if(!addedBones.Contains(t.name))
            {
				BoneInfo info = new BoneInfo();
				info.bone = t;
				info.depth = BoneDepth(info.bone);
				boneRefs.Add(info);
				addedBones.Add(info.bone.name);
			}
			Transform tp = r.transform.parent;
			if (tp)
			{
				if (!addedBones.Contains(tp.name))
				{
					BoneInfo info = new BoneInfo();
					info.bone = tp;
					info.depth = BoneDepth(info.bone);
					boneRefs.Add(info);
					addedBones.Add(info.bone.name);
				}
			}
		}

		//Different meshes may each have a different bindpose...
		//But we're ramming them all into the same exported mesh!
		//We need to duplicate the skeleton to fix it, and must keep the
		//enforced order (I guess I could sort each mesh bonerefs once and then add?)
		if (!forceMeshBindPose)
		{	//Now to sort bones based on depth - makes skeleton updating easier later....
			boneRefs = boneRefs.OrderBy(o => o.depth).ToList();
		}

		ExportMesh();

		if(skinCount > 0)
        {
			if(legacyAnim)
            {
				ExportLegacyAnim();
            }
			else
            {
				ExportAnim();
            }
        }
		ExportMaterial();
	}

    void WriteMatrix(Matrix4x4 m, System.IO.StreamWriter file)
    {
		file.WriteLine(m.m00 + " " + m.m10 + " " + m.m20 + " " + m.m30);
		file.WriteLine(m.m01 + " " + m.m11 + " " + m.m21 + " " + m.m31);
		file.WriteLine(m.m02 + " " + m.m12 + " " + m.m22 + " " + m.m32);
		file.WriteLine(m.m03 + " " + m.m13 + " " + m.m23 + " " + m.m33);
		file.WriteLine("");
	}

	List<Vector3>	allVertPos	= new List<Vector3>();
	List<Vector4>	allTangents	= new List<Vector4>();
	List<Vector3>	allNormals	= new List<Vector3>();
	List<Vector2>	allUVs		= new List<Vector2>();
	List<int>		allIndices	= new List<int>();

	List<int>	allWeightIndices	= new List<int>();
	List<float> allWeightValues		= new List<float>();

	List<string> allJointNames = new List<string>();

	List<Matrix4x4> allJointMatrices = new List<Matrix4x4>();


	List<int>		indexStarts		= new List<int>();
	List<int>		indexEnds		= new List<int>();
	List<string>	meshNames		= new List<string>();

	List<Renderer>		allRenderers	= new List<Renderer>();
	List<Mesh>			allMeshes		= new List<Mesh>();
	List<GameObject>	meshObjects		= new List<GameObject>();

	Dictionary<Material, int> allMats = new Dictionary<Material, int>();

	List<Material> matList	= new List<Material>();
	List<int> matIDs		= new List<int>();

	int meshCount	= 0;
	int skinCount	= 0;
	bool doUVs		= false;
	bool doColours	= false;

	int GetBoneIndex(Transform t)
    {
		for (int i = 0; i < boneRefs.Count; ++i)
		{
			if (boneRefs[i].bone == t)
			{
				return i;
			}
		}
		return -1;
    }

	void PreprocessRenderer(Renderer r, bool skin)
    {
		Mesh m;

		if (skin)
		{
			SkinnedMeshRenderer rr = (SkinnedMeshRenderer)r;
			m = rr.sharedMesh;
		}
		else
		{
			MeshFilter f = r.gameObject.GetComponent<MeshFilter>();
			m = f.sharedMesh;
		}
		Vector2[]	vertUV		= m.uv;
		Color[]		vertColours = m.colors;

		if(vertUV.Length > 0)
        {
			doUVs = true;
        }
		if(vertColours.Length > 0)
        {
			doColours = true;
        }
	}

    void ProcessRenderer(Renderer r, bool skin)
    {
		Mesh m;

		allRenderers.Add(r);

		if (skin)
        {
			SkinnedMeshRenderer rr = (SkinnedMeshRenderer)r;
			m = rr.sharedMesh;
			skinCount++;
		}
		else
        {
			MeshFilter f = r.gameObject.GetComponent<MeshFilter>();
			m = f.sharedMesh;
        }

		allMeshes.Add(m);
		meshObjects.Add(r.gameObject);

		Vector3[] vertPos		= m.vertices;
		Vector3[] vertNormal	= m.normals;
		Vector4[] vertTangent	= m.tangents;
		Vector2[] vertUV		= m.uv;
		Color[]	  vertColors	= m.colors;
		BoneWeight[] weights	= m.boneWeights;

		Debug.Log("Mesh info: "		+ r.name);
		Debug.Log("Vert count: "	+ vertPos.Length);
		Debug.Log("Weights: "		+ weights.Length);

		if (skin && vertPos.Length != weights.Length)
		{
			Debug.LogError("Incorrect bone weight count?");
		}

		int startIndex = allVertPos.Count;

		Matrix4x4 transform = r.gameObject.transform.localToWorldMatrix;

		if(flipZAxis)
        {
			transform = Matrix4x4.Scale(new Vector3(1, 1, -1)) * transform;
        } 

		for (int i = 0; i < vertPos.Length; ++i)
		{
			Vector4 localVert = new Vector4(vertPos[i].x, vertPos[i].y, vertPos[i].z, 1.0f);
			allVertPos.Add(transform * localVert);
			//allVertPos.Add(localVert);
		}

		if (skin)
		{
			SkinnedMeshRenderer rr = (SkinnedMeshRenderer)r;

			int startingJoint = allJointMatrices.Count;

			if(forceMeshBindPose)
            {
				for(int i = 0; i < m.bindposes.Length; ++i)
                {
					allJointMatrices.Add(m.bindposes[i]);
				}
            }

			for (int i = 0; i < weights.Length; ++i)
			{
				allWeightValues.Add(weights[i].weight0);
				allWeightValues.Add(weights[i].weight1);
				allWeightValues.Add(weights[i].weight2);
				allWeightValues.Add(weights[i].weight3);

				if (forceMeshBindPose)
				{
					allWeightIndices.Add(startingJoint + weights[i].boneIndex0);
					allWeightIndices.Add(startingJoint + weights[i].boneIndex1);
					allWeightIndices.Add(startingJoint + weights[i].boneIndex2);
					allWeightIndices.Add(startingJoint + weights[i].boneIndex3);
				}
				else
				{
					int b0 = GetBoneIndex(rr.bones[weights[i].boneIndex0]);
					int b1 = GetBoneIndex(rr.bones[weights[i].boneIndex1]);
					int b2 = GetBoneIndex(rr.bones[weights[i].boneIndex2]);
					int b3 = GetBoneIndex(rr.bones[weights[i].boneIndex3]);

					allWeightIndices.Add(b0);
					allWeightIndices.Add(b1);
					allWeightIndices.Add(b2);
					allWeightIndices.Add(b3);
				}
			}
			//startingJoint += rr.bones.Length;
		}
		else//need to fake it!
		{
			for (int i = 0; i < vertPos.Length; ++i)
			{
				int p = GetBoneIndex(r.transform);

				allWeightValues.Add(1.0f);
				allWeightValues.Add(0.0f);
				allWeightValues.Add(0.0f);
				allWeightValues.Add(0.0f);

				allWeightIndices.Add(p);
				allWeightIndices.Add(0);
				allWeightIndices.Add(0);
				allWeightIndices.Add(0);
			}
		}

		foreach(Vector3 n in vertNormal)
        {
			allNormals.Add(transform * n);
		}
		foreach (Vector4 t in vertTangent)
		{
			allTangents.Add(transform * t);
		}

		if (vertUV.Length > 0)
		{
			allUVs.AddRange(vertUV);
		}
		else if(doUVs)
        {
			for(int i = 0; i < vertPos.Length; ++i)
            {
				allUVs.Add(Vector2.zero);
            }
        }
		
		Material[] sm		= r.sharedMaterials;
		int writeCount		= Mathf.Min(sm.Length, m.subMeshCount);
		int overflowCount	= Mathf.Max(0, m.subMeshCount - writeCount); //weird mesh setups where there's more mesh layers than mat laters...

		for(int i = 0; i < writeCount; ++i)
        {
			int id = -1;
			for(int j = 0; j < matList.Count; ++j)
            {
				if(sm[i] == matList[j])
                {
					id = j;
					break;
                }
            }
			if(id == -1)
            {
				matList.Add(sm[i]);
				id = matList.Count - 1;
			}
			matIDs.Add(id);
		}
		for(int i = 0; i < overflowCount; ++i)
        {
			matIDs.Add(-1);
        }

		for (int i = 0; i < m.subMeshCount; ++i)
		{
			meshNames.Add(m.name+"_"+i);

			int[] indices = m.GetIndices(i);
			Debug.Log("Index count: " + indices.Length);
			int start = allIndices.Count;
			indexStarts.Add(start);
			foreach (int index in indices)
			{
				allIndices.Add(startIndex + index);
			}
			indexEnds.Add(allIndices.Count - start);

			meshCount++;
		}
	}

	void ExportMesh() {
		Animator anim = GetComponent<Animator>();

		int expectedAttribCount = 0;
		int exportedAttribCount = 0;

		using (System.IO.StreamWriter file =
			new System.IO.StreamWriter(this.name + ".msh", false)) {

			foreach (SkinnedMeshRenderer r in skinnedRenderers)
			{
				PreprocessRenderer(r, true);
			}
			foreach (MeshRenderer r in renderers)
			{
				PreprocessRenderer(r, false);
			}


			foreach (SkinnedMeshRenderer r in skinnedRenderers)
			{
				ProcessRenderer(r, true);
			}
			foreach (MeshRenderer r in renderers)
			{
				ProcessRenderer(r, false);
			}

			expectedAttribCount += allVertPos.Count > 0 ? 1 : 0;
			expectedAttribCount += allNormals.Count > 0 ? 1 : 0;
			expectedAttribCount += allTangents.Count > 0 ? 1 : 0;
			expectedAttribCount += allUVs.Count > 0 ? 1 : 0;
			expectedAttribCount += allIndices.Count > 0 ? 1 : 0;
			expectedAttribCount += indexStarts.Count > 0 ? 1 : 0;
			expectedAttribCount += meshNames.Count > 0 ? 1 : 0;

			if (skinCount > 0)
			{
				expectedAttribCount += 6;
			}

			file.WriteLine("MeshGeometry");
			file.WriteLine(1);
			file.WriteLine(meshCount);
			file.WriteLine(allVertPos.Count);
			file.WriteLine(allIndices.Count);
			file.WriteLine(expectedAttribCount);

			if (allVertPos.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.VPositions); exportedAttribCount++;
				foreach (Vector3 v in allVertPos)
				{
					file.WriteLine(v.x + " " + v.y + " " + v.z);
				}
			}
			if (allNormals.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.VNormals); exportedAttribCount++;
				foreach (Vector3 v in allNormals)
				{
					file.WriteLine(v.x + " " + v.y + " " + v.z);
				}
			}

			if (allTangents.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.VTangents); exportedAttribCount++;
				foreach (Vector4 v in allTangents)
				{
					file.WriteLine(v.x + " " + v.y + " " + v.z + " " + v.w);
				}
			}

			if (allUVs.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.VTex0); exportedAttribCount++;
				foreach (Vector2 v in allUVs)
				{
					file.WriteLine(v.x + " " + v.y);
				}
			}
			if (allIndices.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.Indices); exportedAttribCount++;
				for (int i = 0; i < allIndices.Count; i += 3)
				{
					if(flipZAxis)
                    {
						file.WriteLine(allIndices[i] + " " + allIndices[i + 2] + " " + allIndices[i + 1]);
                    }
					else
                    {
						file.WriteLine(allIndices[i] + " " + allIndices[i + 1] + " " + allIndices[i + 2]);
					}
				}
			}

			if (indexStarts.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.SubMeshes); exportedAttribCount++;
				for (int i = 0; i < indexStarts.Count; ++i)
				{
					file.WriteLine(indexStarts[i] + " " + indexEnds[i]);
				}
			}
			if (meshNames.Count > 0)
			{
				file.WriteLine((int)GeometryChunkTypes.SubMeshNames); exportedAttribCount++;
				foreach (string s in meshNames)
				{
					file.WriteLine(s);
				}
			}
			if (skinCount > 0) 
			{
				file.WriteLine((int)GeometryChunkTypes.VWeightValues); exportedAttribCount++;
				for (int i = 0; i < allWeightValues.Count; i += 4)
				{
					file.WriteLine(allWeightValues[i] + " " + allWeightValues[i + 1] + " " + allWeightValues[i + 2] + " " + allWeightValues[i + 3]);
				}

				file.WriteLine((int)GeometryChunkTypes.VWeightIndices); exportedAttribCount++;
				for (int i = 0; i < allWeightIndices.Count; i += 4)
				{
					file.WriteLine(allWeightIndices[i] + " " + allWeightIndices[i + 1] + " " + allWeightIndices[i + 2] + " " + allWeightIndices[i + 3]);
				}

				file.WriteLine((int)GeometryChunkTypes.JointNames); exportedAttribCount++;
				file.WriteLine(boneRefs.Count);
				foreach (BoneInfo t in boneRefs)
				{
					file.WriteLine(t.bone.name);
				}

				file.WriteLine((int)GeometryChunkTypes.JointParents); exportedAttribCount++;
				file.WriteLine(boneRefs.Count);

				foreach (BoneInfo t in boneRefs)
				{
					int p = -1;
					if (t.bone.parent != null)
					{
						p = GetBoneIndex(t.bone.parent);
					}
					file.WriteLine(p);
				}

				file.WriteLine((int)GeometryChunkTypes.BindPose); exportedAttribCount++;
				file.WriteLine(boneRefs.Count);
				if (forceMeshBindPose)
				{
					foreach (Matrix4x4 mat in allJointMatrices)
					{
						Matrix4x4 m = mat;
						if (flipZAxis)
						{
							m = Matrix4x4.Scale(new Vector3(1, 1, -1)) * m;
						}
						m = m.inverse;
						WriteMatrix(m, file);
					}
				}
				else
				{
					foreach (BoneInfo t in boneRefs)
					{
						Matrix4x4 m = t.bone.localToWorldMatrix;
						if (flipZAxis)
						{
							m = Matrix4x4.Scale(new Vector3(1, 1, -1)) * m;
						}

						WriteMatrix(m, file);
					}
				}

				file.WriteLine((int)GeometryChunkTypes.BindPoseInv); exportedAttribCount++;
				file.WriteLine(boneRefs.Count);
				if (forceMeshBindPose)
				{
					foreach (Matrix4x4 mat in allJointMatrices)
					{
						Matrix4x4 m = mat;
						if (flipZAxis)
						{
							m = Matrix4x4.Scale(new Vector3(1, 1, -1)) * m;
						}
						WriteMatrix(m, file);
					}
				}
				else
				{
					foreach (BoneInfo t in boneRefs)
					{
						Matrix4x4 m = t.bone.worldToLocalMatrix;
						if (flipZAxis)
						{
							m = t.bone.localToWorldMatrix;
							m = (Matrix4x4.Scale(new Vector3(1, 1, -1)) * m).inverse;
						}

						WriteMatrix(m, file);
					}
				}
			}
		}
		if(exportedAttribCount != expectedAttribCount)
        {
			Debug.LogWarning("Mesh exported incorrect attribute count?");
        }
	}

	void ExportLegacyAnim()
    {
		Animation a = GetComponent<Animation>();
		if (!a)
		{
			return;
		}
		float animLength	= a.clip.length;
		float animRate		= a.clip.frameRate;

		int		numFrames = (int)(animRate * animLength);
		float	frameTime = animLength / numFrames;

		string filename = animName;
		if (filename.Length == 0)
		{
			filename = this.name;
		}

		using (System.IO.StreamWriter file = new System.IO.StreamWriter(filename + ".anm", false))
		{
			file.WriteLine("MeshAnim");
			file.WriteLine(1);
			file.WriteLine(numFrames);
			file.WriteLine(boneRefs.Count);
			file.WriteLine(animRate);

			for (int i = 0; i < numFrames; ++i)
			{
				float realT = frameTime * i;

				a.clip.SampleAnimation(this.gameObject, realT);

				for (int j = 0; j < boneRefs.Count; ++j)
				{
					a.clip.SampleAnimation(boneRefs[j].bone.gameObject, realT);
					Matrix4x4 m = boneRefs[j].bone.localToWorldMatrix;

					if (flipZAxis)
					{
						m = Matrix4x4.Scale(new Vector3(1, 1, -1)) * m;
					}
					WriteMatrix(m, file);
				}
			}
		}
	}

	void ExportAnim()
	{
		Animator a = GetComponent<Animator>();

		if(!a)
        {
			return;
        }
		AnimatorClipInfo[] infos = a.GetCurrentAnimatorClipInfo(0);

		float	animLength	= infos[0].clip.length;
		float	animRate	= infos[0].clip.frameRate;

		int		numFrames	= (int)(animRate * animLength);
		float	frameTime	= animLength / numFrames;

		string filename = animName;
		if (filename.Length == 0)
        {
			filename = this.name;
        }

		using (System.IO.StreamWriter file = new System.IO.StreamWriter(filename + ".anm", false))
		{
			file.WriteLine("MeshAnim");
			file.WriteLine(1);
			file.WriteLine(numFrames);
			file.WriteLine(boneRefs.Count);
			file.WriteLine(animRate);

			for(int i = 0; i < numFrames; ++i)
            {
				float realT = frameTime * i;

				infos[0].clip.SampleAnimation(this.gameObject, realT);

				for (int j = 0; j < boneRefs.Count; ++j) {
					infos[0].clip.SampleAnimation(boneRefs[j].bone.gameObject, realT);
					Matrix4x4 m = boneRefs[j].bone.localToWorldMatrix;

					if(flipZAxis)
                    {
						m =  Matrix4x4.Scale(new Vector3(1, 1, -1)) * m;
                    }

					WriteMatrix(m, file);
				}	
			}
		}
	}

	void ExportMaterial()
	{
		int meshCount = matIDs.Count;

        using (System.IO.StreamWriter file = new System.IO.StreamWriter(this.name + ".mat", false))
        {
            file.WriteLine("MeshMat");
            file.WriteLine(1);
			file.WriteLine(matList.Count);
			file.WriteLine(matIDs.Count);

			foreach(Material m in matList)
            {
				Texture diffTex		= m.mainTexture;
				Texture bumpTex		= m.HasProperty("_BumpMap") ? m.GetTexture("_BumpMap") : null;
				Texture metTex		= m.HasProperty("_MetallicGlossMap")   ? m.GetTexture("_MetallicGlossMap")   : null;
				Texture heightTex	= m.HasProperty("_ParallaxMap") ? m.GetTexture("_ParallaxMap") : null;

				int texCount = 0;
				if (diffTex)	texCount++;
				if (bumpTex)	texCount++;
				if (metTex)		texCount++;
				if (heightTex)	texCount++;

				file.WriteLine(m.name);
				file.WriteLine(texCount);
				//Substring to cut off the 'Assets' bit...
				if(diffTex)
                {
					string path = AssetDatabase.GetAssetPath(diffTex);
					file.WriteLine("Diffuse:" + path.Substring(6));
                }
				if (bumpTex)
				{
					string path = AssetDatabase.GetAssetPath(bumpTex);
					file.WriteLine("Bump:" + path.Substring(6));
				}
				if (metTex)
				{
					string path = AssetDatabase.GetAssetPath(metTex);
					file.WriteLine("Metallic:" + path.Substring(6));
				}
				if (heightTex)
				{
					string path = AssetDatabase.GetAssetPath(heightTex);
					file.WriteLine("Height:" + path.Substring(6));
				}
			}
			for (int j = 0; j < matIDs.Count; ++j)
            {
				file.WriteLine(matIDs[j]);
			}
        }
    }
}
