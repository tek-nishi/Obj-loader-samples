//
// OBJ形式を扱う４
//   頂点座標、法線ベクトルに対応
//   mtlファイルに対応
//

// TIPS:M_PIとかを使えるようにする
#define _USE_MATH_DEFINES

#include <cmath>
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <iostream>
#include <vector>
#include <map>

// GLFWのヘッダ内で他のライブラリを取り込む
#define GLFW_INCLUDE_GLEXT
#define GLFW_INCLUDE_GLU

#if defined(_MSC_VER)
// Windows:GLEWをスタティックライブラリ形式で利用
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
// Windows:外部ライブラリのリンク指定
#if defined (_DEBUG)
#pragma comment(lib, "glfw3d.lib")
#pragma comment(lib, "glew32sd.lib")
#else
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "glew32s.lib")
#endif
#pragma comment(lib, "OPENGL32.lib")
#pragma comment(lib, "GLU32.lib")
#endif


// マテリアル
// TIPS メンバ変数にあらかじめ初期値を与えている
struct Material {
  GLfloat ambient[4]  = { 0.5f, 0.5f, 0.5f, 1.0f };
  GLfloat diffuse[4]  = { 0.5f, 0.5f, 0.5f, 1.0f };
  GLfloat specular[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
  GLfloat shininess   = 80.0f;
};

// 読み込んだOBJ形式の情報
// NOTICE:OpenGLにそのまま渡してglDrawArraysで描画する
//        構造になっている
struct Mesh {
  std::vector<GLfloat> vtx;
  std::vector<GLfloat> normal;

  bool has_normal;
  std::string mat_name;
};

struct Obj {
  std::vector<Mesh> mesh;
  std::map<std::string, Material> material;
};

// 面を構成する頂点情報
struct Face {
  int vi[3];           // 頂点番号
  int ni[3];           // 法線ベクトル番号
};


// マテリアルを読み込む
std::map<std::string, Material> loadMaterial(const std::string& path) {
  std::map<std::string, Material> material;

  std::ifstream stream(path);
  // ファイルを開けなければ、空の情報を返す
  if (!stream) return material; 

  std::string cur_name;

  while (!stream.eof()) {
    std::string s;
    std::getline(stream, s);

    std::stringstream ss(s);
    std::string key;
    ss >> key;
    if (key == "newmtl") {
      ss >> cur_name;
    }
    else if (key == "Ns") {
      // スペキュラ指数
      ss >> material[cur_name].shininess;
    }
    else if (key == "Tr") {
      // 透過
    }
    else if (key == "Kd") {
      // 拡散光
      ss >> material[cur_name].diffuse[0]
         >> material[cur_name].diffuse[1]
         >> material[cur_name].diffuse[2];
    }
    else if (key == "Ks") {
      // スペキュラ
      ss >> material[cur_name].specular[0]
         >> material[cur_name].specular[1]
         >> material[cur_name].specular[2];
    }
    else if (key == "Ka") {
      // 環境光
      ss >> material[cur_name].ambient[0]
         >> material[cur_name].ambient[1]
         >> material[cur_name].ambient[2];
    }
  }

  return material;
}


// OBJ形式を読み込む
Obj loadObj(const std::string& path) {
  std::ifstream stream(path);
  assert(stream);

  // ファイルから読み取った値を一時的に保持しておく変数
  std::vector<GLfloat> vtx;
  std::vector<GLfloat> normal;
  std::vector<GLfloat> texture;
  std::map<std::string, std::vector<Face>> face;
  std::string cur_mat("");       // マテリアル名

  // 変換後の値
  Obj obj;

  // NOTICE OBJ形式はデータの並びに決まりがないので、
  //        いったん全ての情報をファイルから読み取る
  while (!stream.eof()) {
    std::string s;
    std::getline(stream, s);

    // TIPS:文字列ストリームを使い
    //      文字列→数値のプログラム負荷を下げている
    std::stringstream ss(s);

    std::string key;
    ss >> key;
    if (key == "mtllib") {
      // マテリアル
      std::string m_path;
      ss >> m_path;
      obj.material = loadMaterial(m_path);
    }
    else if (key == "usemtl") {
      // 適用するマテリアルを変更
      ss >> cur_mat;
    }
    else if (key == "v") {
      // 頂点座標
      float x, y, z;
      ss >> x >> y >> z;
      vtx.push_back(x);
      vtx.push_back(y);
      vtx.push_back(z);
    }
    else if (key == "vt") {
      // 頂点テクスチャ座標
      float u, v;
      ss >> u >> v;
      texture.push_back(u);
      texture.push_back(v);
    }
    else if (key == "vn") {
      // 頂点法線ベクトル
      float x, y, z;
      ss >> x >> y >> z;
      normal.push_back(x);
      normal.push_back(y);
      normal.push_back(z);
    }
    else if (key == "f") {
      // 面を構成する頂点情報
      // FIXME ３角形のみの対応

      // TIPS 初期値を-1にしておき、
      //      「データが存在しない」状況に対応
      Face f = {
        { -1, -1, -1 },
        { -1, -1, -1 }
      };

      for (int i = 0; i < 3; ++i) {
        std::string text;
        ss >> text;
        // TIPS '/' で区切られた文字列から数値に変換するため
        //      一旦stringstreamへ変換
        std::stringstream fs(text);

        // 面を構成する頂点番号、法線ベクトル番号を取り出す
        {
          std::string v;
          std::getline(fs, v, '/');
          f.vi[i] = std::stoi(v) - 1;
        }
        {
          std::string v;
          std::getline(fs, v, '/');
          // テクスチャ座標は使わない
        }
        {
          std::string v;
          std::getline(fs, v, '/');
          if (!v.empty()) {
            f.ni[i] = std::stoi(v) - 1;
          }
        }
      }
      face[cur_mat].push_back(f);
    }
  }

  // 読み込んだ面情報から最終的な頂点配列を作成
  for (const auto& f : face) {
    Mesh mesh;
    // マテリアル名 
    mesh.mat_name   = f.first;
    mesh.has_normal = f.second[0].ni[0] >= 0;

    // 頂点情報
    for (const auto& m : f.second) {
      for (int i = 0; i < 3; ++i) {
        {
          // 頂点座標
          int index = m.vi[i] * 3;
          for (int j = 0; j < 3; ++j) {
            mesh.vtx.push_back(vtx[index + j]);
          }
        }
        if (mesh.has_normal) {
          // 法線ベクトル
          int index = m.ni[i] * 3;
          for (int j = 0; j < 3; ++j) {
            mesh.normal.push_back(normal[index + j]);
          }
        }
      }
    }

    obj.mesh.push_back(mesh);
  }

  return obj;
}


// マテリアル設定
void setupMaterial(const Material& material) {
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material.ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material.diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material.specular);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material.shininess);
  
  // 自己発光
  GLfloat mat_emi[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emi);
}

// メッシュ描画
void drawMesh(const Mesh& mesh) {
  glVertexPointer(3, GL_FLOAT, 0, &mesh.vtx[0]);
  glEnableClientState(GL_VERTEX_ARRAY);

  if (mesh.has_normal) {
    // NOTICE 法線ベクトルが有効かどうかで処理が変わる
    glNormalPointer(GL_FLOAT, 0, &mesh.normal[0]);
    glEnableClientState(GL_NORMAL_ARRAY);
  }
  else {
    glDisableClientState(GL_NORMAL_ARRAY);
  }

  glDrawArrays(GL_TRIANGLES, 0, mesh.vtx.size() / 3);
}


int main() {
  // 初期化
  if (!glfwInit()) return -1;

  // Window生成
  GLFWwindow* window = glfwCreateWindow(800,
                                        600,
                                        "OBJ loader",
                                        nullptr, nullptr);

  if (!window) {
    // 生成失敗
    glfwTerminate();
    return -1;
  }

  // OpenGLの命令を使えるようにする
  glfwMakeContextCurrent(window);
  // アプリ画面更新タイミングをPCディスプレイに同期する
  glfwSwapInterval(1);

#if defined (_MSC_VER)
  // GLEWを初期化(Windows)
  if (glewInit() != GLEW_OK) {
    // パソコンがOpenGL拡張に対応していなければ終了
    glfwTerminate();
    return -1;
  }
#endif

  // これ以降OpenGLの命令が使える
  
  Obj obj = loadObj("box.obj");

  // 拡散光と鏡面反射を個別に計算する
  // TIPS:これで、テクスチャを張ったポリゴンもキラーン!!ってなる
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
  
  // 透視変換行列を設定
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(35.0, 640 / 480.0, 0.2, 200.0);

  // 操作対象の行列をモデリングビュー行列に切り替えておく
  glMatrixMode(GL_MODELVIEW);

  float r = 0.0;
  
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // 並行光源の設定
  GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

  GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

  GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

  while (!glfwWindowShouldClose(window)) {
    // 描画バッファの初期化
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 単位行列を読み込む
    glLoadIdentity();

    // ライトの設定
    GLfloat position[] = { 0.0f, 0.0f, 4.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glTranslatef(0, 0, -5.0);
    glRotatef(r, 0, 1, 0);
    glRotatef(r, 1, 0, 0);
    r += 1.0;

    // 描画
    // TIPS マテリアルごとにグループ分けして描画
    for (const auto& mesh : obj.mesh) {
      setupMaterial(obj.material[mesh.mat_name]);
      drawMesh(mesh);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  
  glfwTerminate();
}
