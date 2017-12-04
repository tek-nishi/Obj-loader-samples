//
// OBJ形式を扱う２
//   頂点座標と法線ベクトルに対応 
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


// 読み込んだOBJ形式の情報
// NOTICE:OpenGLにそのまま渡してglDrawArraysで描画する
//        構造になっている
struct Obj {
  std::vector<GLfloat> vtx;
  std::vector<GLfloat> normal;

  bool has_normal;
};

// 面を構成する頂点情報
struct Face {
  int vi[3];           // 頂点番号
  int ni[3];           // 法線ベクトル番号
};

// OBJ形式を読み込む
Obj loadObj(const std::string& path) {
  std::ifstream stream(path);
  assert(stream);

  // ファイルから読み取った値を一時的に保持しておく変数
  std::vector<GLfloat> vtx;
  std::vector<GLfloat> normal;
  std::vector<Face>    face;

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
    if (key == "v") {
      // 頂点座標
      float x, y, z;
      ss >> x >> y >> z;
      vtx.push_back(x);
      vtx.push_back(y);
      vtx.push_back(z);
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
          // NOTICE テクスチャ座標は使わない
        }
        {
          std::string v;
          std::getline(fs, v, '/');
          if (!v.empty()) {
            f.ni[i] = std::stoi(v) - 1;
          }
        }
      }
      face.push_back(f);
    }
  }

  // NOTICE 法線ベクトルが有効な場合は、配列の要素番号が０以上になっている
  obj.has_normal = face[0].ni[0] >= 0;

  // 読み込んだ面情報から最終的な頂点配列を作成
  for (auto& f : face) {
    for (int i = 0; i < 3; ++i) {
      {
        // 頂点座標
        int index = f.vi[i] * 3;
        for (int j = 0; j < 3; ++j) {
          obj.vtx.push_back(vtx[index + j]);
        }
      }
      if (obj.has_normal) {
        // 法線ベクトル
        int index = f.ni[i] * 3;
        for (int j = 0; j < 3; ++j) {
          obj.normal.push_back(normal[index + j]);
        }
      }
    }
  }

  return obj;
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
  
  // box_v.obj     頂点座標のみ有効
  // box_vn.obj    頂点座標と法線ベクトルが有効
  // box_vt.obj    頂点座標とテクスチャ座標が有効
  // box_vtn.obj   頂点座標、テクスチャ座標、法線ベクトルが有効
  Obj obj = loadObj("box_vtn.obj");

  glVertexPointer(3, GL_FLOAT, 0, &obj.vtx[0]);
  glEnableClientState(GL_VERTEX_ARRAY);

  if (obj.has_normal) {
    // NOTICE 法線ベクトルが有効かどうかで処理が変わる
    glNormalPointer(GL_FLOAT, 0, &obj.normal[0]);
    glEnableClientState(GL_NORMAL_ARRAY);
  }

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
    glDrawArrays(GL_TRIANGLES, 0, obj.vtx.size() / 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
}
