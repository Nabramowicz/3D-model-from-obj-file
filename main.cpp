#include "include.h"


/* Program pozwala odczytywać pliki typu .obj, które:
* 1) posiadają wektory normalne (linijki zaczynające się od "vn" w pliku),
* 2) face (linijki zaczynające się od "f" w pliku) składa sie z trzech elementów (triangles),
* 3) tekstura wymaga rozszerzenia .bmp
* 
* Przykładowe pliki do odtworzenia znajdują się w katalogu "objFiles"; 
* 
* Program powstał w oparciu o wiedzę i tutoriale z zajęć Grafika komputerowa 3D, 
* a także w opraciu o tutorial ze strony http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/.
*
* Autorka: Natalia Abramowicz
* Nr albumu: 410635
*/


//elementy wykorzystywane przy oświetleniu
GLuint objectColor_id = 0;
GLuint lightColor_id = 0;
GLuint lightPos_id = 0;
GLuint viewPos_id = 0;

//Wymiary okna
int screen_width = 1200;
int screen_height = 780;

//Mysz 
int pozycjaMyszyX; 
int pozycjaMyszyY;
//Wciśnięty klawisz
int mbutton; 

//Widok
double kameraX= 0.0;
double kameraZ = 20.0;
double kameraD = -6.0;
double kameraPredkosc;
double kameraKat = 20;
double kameraPredkoscObrotu;
double poprzednie_kameraX;
double poprzednie_kameraZ;
double poprzednie_kameraD;

double rotation = 0;

//Macierze
glm::mat4 MV; //modelview - macierz modelu i świata
glm::mat4 P;  //projection - macierz projekcji, czyli naszej perspektywy

//położenie oświetlenia
glm::vec3 lightPos(0.0f, 10.0f, 0.0f);

//Wektory do poszczególnych elementów z pliku .obj
std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;
std::vector< glm::vec3 > colors;
std::vector< float > colorsy;

/*###############################################################*/

//Funkcja odczytująca pliku typu .obj i zapisywanie danych do odpowiednich wektorów
bool loadOBJ(
	const char* path,
	std::vector < glm::vec3 >& out_vertices,
	std::vector < glm::vec2 >& out_uvs,
	std::vector < glm::vec3 >& out_normals
) {
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	//otworzenie pliku do czytania 
	FILE* file;
	errno_t err;
	if ((err = fopen_s(&file, path, "r")) != 0) {
		printf("Pliku nie udało się otworzyć!\n");
		return false;
	}

	//odczytywanie pliku obj
	while (1) {
		char lineHeader[200];
		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));

		if (res == EOF) 
			break;
		else
		{
			//vertex coordinates
			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}

			//texture coordinates
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
				temp_uvs.push_back(uv);
			}

			//normals 
			else if (strcmp(lineHeader, "vn") == 0) {
				glm::vec3 normal;
				fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				temp_normals.push_back(normal);
			}

			//face (vertex coords id, texture coords id, normals id)
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					printf("Plik jest niezgodny z zaleceniami wymienionymi w komentarzu na wstępie!\n");
					return false;
				}
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
		}
	}

	for (unsigned int i = 0; i < std::size(vertexIndices); i++) {
		glm::vec3 vertex = temp_vertices[vertexIndices[i] - 1];
		out_vertices.push_back(vertex);
	}
	for (unsigned int i = 0; i < std::size(uvIndices); i++) {
		glm::vec2 uv = temp_uvs[uvIndices[i] - 1];
		out_uvs.push_back(uv);
	}
	for (unsigned int i = 0; i < std::size(normalIndices); i++) {
		glm::vec3 normal = temp_normals[normalIndices[i] - 1];
		out_normals.push_back(normal);
	}
}

/*###############################################################*/

//Funkcja przypisująca odpowiednie kolory w zależności od współrzędnej y vertices (wysokość)
void colors_for_vert(std::vector< glm::vec3 > vertices, std::vector< glm::vec3 >& cols) {
	for (int i = 0; i < vertices.size(); i++) {
		colorsy.push_back(vertices[i].y); //współrzędne y vertices 
	}
	auto min = min_element(std::begin(colorsy), std::end(colorsy));
	auto max = max_element(std::begin(colorsy), std::end(colorsy));
	float rel_max = max[0] - min[0]; 

	//znormalizowane wartości wysokości
	for (int i = 0; i < colorsy.size(); i++) {
		colorsy[i] = (colorsy[i] - min[0]) / rel_max;
	}
	//przypisywanie kolorów, tak aby płynnie przechodziły od niebieskiego przez zielony i żółty, aż do czerwonego
	for (int i = 0; i < vertices.size(); i++) {
		if (colorsy[i] < 0.25) 
			cols.push_back({ 0.0f, colorsy[i] * 4, 1.0f });
		else if (colorsy[i] < 0.5)
			cols.push_back({ 0.0f, 1.0f, (1.0 - colorsy[i] - 0.5) * 4 });
		else if (colorsy[i] < 0.75)
			cols.push_back({ (colorsy[i] - 0.5 ) * 4, 1.0f, 0.0f });
		else
			cols.push_back({ 1.0f, (1.0 - colorsy[i]) * 4, 0.0f });
	}

}

/*###############################################################*/

GLuint programID = 0;
GLuint programID1 = 0;
GLuint programID2 = 0;
GLuint programID3 = 0;
GLuint tex_id;
GLint uniformTex;

unsigned int VBO, VBOnorm, VBOcol, VBOtex;
unsigned int VAO[4];


/*###############################################################*/

void mysz(int button, int state, int x, int y)
{
	mbutton = button;
	switch (state)
	{
	case GLUT_UP:
		break;
	case GLUT_DOWN:
		pozycjaMyszyX = x;
		pozycjaMyszyY = y;
		poprzednie_kameraX = kameraX;
		poprzednie_kameraZ = kameraZ;
		poprzednie_kameraD = kameraD;
		break;

	}
}

/*###############################################################*/

void mysz_ruch(int x, int y)
{
	if (mbutton == GLUT_LEFT_BUTTON)
	{
		kameraX = poprzednie_kameraX - (pozycjaMyszyX - x) * 0.1;
		kameraZ = poprzednie_kameraZ - (pozycjaMyszyY - y) * 0.1;
	}
	if (mbutton == GLUT_RIGHT_BUTTON)
	{
		kameraD = poprzednie_kameraD + (pozycjaMyszyY - y) * 0.1;
	}

}

/*###############################################################*/

double dx = 0;
double dy = 0;
double dz = 0;
int program_id = 0;
int day_or_night = 0;

/*###############################################################*/

void klawisz(GLubyte key, int x, int y)
{
	switch (key) {

	case 27:    // Esc - koniec 
		exit(1);
		break;

	case 32:    // SPACE - nastepny widok
		if (program_id == 0) {
			program_id = 1;
		}
		else if (program_id == 1) {
			program_id = 2;
		}
		else if (program_id == 2) {
			program_id = 3;
		}
		else if (program_id == 3) {
			program_id = 0;
		}
		break;

	case 13:    // ENTER - zmiana oświetlenia
		if (day_or_night == 0) {
			day_or_night = 1;
		}
		else if (day_or_night == 1) {
			day_or_night = 0;
		}
		break;


	//zmiana pozycji przy pomocy klwiszy A, W, S, D, +, -:

	case 'w': //do przodu
		dz += 0.2;
		break;

	case 's': //do tyłu
		dz -= 0.2;
		break;

	case 'a': //w lewo
		dx += 0.2;
		break;

	case 'd': //w prawo
		dx -= 0.2;
		break;

	case 43: //+, w górę
		dy -= 0.2;
		break;

	case 45: //-, w dół
		dy += 0.2;
		break;
	}

	
}

/*###############################################################*/

void rysuj(void)
{

	GLfloat color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glClearBufferfv(GL_COLOR, 0, color);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Kasowanie ekranu

	MV = glm::mat4(1.0f);  //macierz jednostkowa
	MV = glm::translate(MV, glm::vec3(0, 0, kameraD));
	MV = glm::rotate(MV, (float)glm::radians(kameraZ), glm::vec3(1, 0, 0));
	MV = glm::rotate(MV, (float)glm::radians(kameraX), glm::vec3(0, 1, 0));

	glm::mat4 MVP = P * MV;
	GLuint MVP_id;
	GLfloat attrib[3];

	//zmiana pozycji
	MVP = glm::translate(MV, glm::vec3(0.0 + dx, 0.0 + dy, 0.0 + dz));
	MVP = P * MVP;

	//przełączanie między programami za pomocą spacji
	switch (program_id){
	case 0:
		glUseProgram(programID);

		MVP_id = glGetUniformLocation(programID, "MVP"); 
		glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(MVP[0][0]));	  	

		//kolory siatki
		attrib[0] = 0.3f;
		attrib[1] = 0.7f;
		attrib[2] = 0.1f; 
		glVertexAttrib3fv(1, attrib);
	
		glBindVertexArray(VAO[0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, 3 * vertices.size());
		glLineWidth(0.3);

		break;

	case 1:
		glUseProgram(programID1);

		MVP_id = glGetUniformLocation(programID1, "MVP"); 
		glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(MVP[0][0]));	 	

		glBindVertexArray(VAO[1]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, 3 * vertices.size());

		
		switch (day_or_night) {
		case 0: //day

			glUniform3f(objectColor_id, 0.5f, 0.5f, 0.5f);
			glUniform3f(lightColor_id, 1.0f, 1.0f, 1.0f);
			glUniform3f(lightPos_id, lightPos[0], lightPos[1], lightPos[2]);

			break;

		case 1: //night

			glUniform3f(objectColor_id, 0.5f, 0.5f, 0.5f);
			glUniform3f(lightColor_id, 0.2f, 0.2f, 0.2f);
			glUniform3f(lightPos_id, lightPos[0], lightPos[1], lightPos[2]);

			break;
		}
		
		break;

	case 2:
		glUseProgram(programID2);

		MVP_id = glGetUniformLocation(programID2, "MVP"); 
		glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(MVP[0][0]));	

		glBindVertexArray(VAO[2]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, 3 * vertices.size());

		glUniform3f(lightColor_id, 1.0f, 1.0f, 1.0f);
		glUniform3f(lightPos_id, lightPos[0], lightPos[1], lightPos[2]);

		break;

	case 3:
		glUseProgram(programID3);

		MVP_id = glGetUniformLocation(programID3, "MVP"); 
		glUniformMatrix4fv(MVP_id, 1, GL_FALSE, &(MVP[0][0]));	  

		glBindVertexArray(VAO[3]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		glUniform1i(uniformTex, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3 * vertices.size());

		switch (day_or_night) {
		case 0: //day

			glUniform3f(lightColor_id, 1.0f, 1.0f, 1.0f);
			glUniform3f(lightPos_id, lightPos[0], lightPos[1], lightPos[2]);

			break;

		case 1: //night

			glUniform3f(lightColor_id, 0.2f, 0.2f, 0.2f);
			glUniform3f(lightPos_id, lightPos[0], lightPos[1], lightPos[2]);

			break;
		}


		break;
	}


	glFlush();
	glutSwapBuffers();

}
/*###############################################################*/

void rozmiar(int width, int height)
{
	screen_width = width;
	screen_height = height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screen_width, screen_height);

	P = glm::perspective(glm::radians(60.0f), (GLfloat)screen_width / (GLfloat)screen_height, 1.0f, 1000.0f);

	glutPostRedisplay(); // Przerysowanie sceny
}

/*###############################################################*/

void idle()
{

	glutPostRedisplay();
}

/*###############################################################*/

GLfloat k = 0.05;
GLfloat ad = 0.0;

void timer(int value) {

	glutTimerFunc(20, timer, 0);
}

/*###############################################################*/

int main(int argc, char **argv)
{
	bool obj = loadOBJ("objFiles/icelandic_mountain/IcelandicMountain.obj", vertices, uvs, normals);
	colors_for_vert(vertices, colors);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(screen_width, screen_height);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) / 2) - screen_width/2, (glutGet(GLUT_SCREEN_HEIGHT) / 2) - screen_height/2);
	glutCreateWindow("objFile Loader");

	glewInit();

	glutDisplayFunc(rysuj);		// def. funkcji rysującej
    glutIdleFunc(idle);			
	glutTimerFunc(20, timer, 0);
	glutReshapeFunc(rozmiar); 
									
	glutKeyboardFunc(klawisz);		//obsługa klawiatury
	//obsługa myszy
	glutMouseFunc(mysz); 	
	glutMotionFunc(mysz_ruch); 

	glEnable(GL_DEPTH_TEST);

	//4 widoki, więc 4 VAO
	glGenVertexArrays(4, VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &VBOnorm);
	glBindBuffer(GL_ARRAY_BUFFER, VBOnorm);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &VBOcol);
	glBindBuffer(GL_ARRAY_BUFFER, VBOcol);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);

	glGenBuffers(1, &VBOtex);
	glBindBuffer(GL_ARRAY_BUFFER, VBOtex);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	//program z modelem z siatki trójkątów
	programID = loadShaders("vertex_shader.glsl", "fragment_shader.glsl");

	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//program z modelem 3D i oświetleniem
	programID1 = loadShaders("vertex_shader.glsl", "fragment_shader_light.glsl");

	objectColor_id = glGetUniformLocation(programID1, "objectColor");
	lightColor_id = glGetUniformLocation(programID1, "lightColor");
	lightPos_id = glGetUniformLocation(programID1, "lightPos");

	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //to samo VBO, ponieważ nadal chce wykorzystywac wierzchołki
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOnorm);  //VBO wektorów normalnych
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//program z pokolorowanym modelem w zależności od wysokości
	programID2 = loadShaders("vertex_shader.glsl", "fragment_shader_col.glsl");

	lightColor_id = glGetUniformLocation(programID2, "lightColor");
	lightPos_id = glGetUniformLocation(programID2, "lightPos");

	glBindVertexArray(VAO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //to samo VBO, ponieważ nadal chce wykorzystywac wierzchołki
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOnorm); //VBO wektorów normalnych
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOcol); //VBO kolorów
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


	//program z modelem pokrytym teksturą
	programID3 = loadShaders("vertex_shader.glsl", "fragment_shader_tex.glsl");

	lightColor_id = glGetUniformLocation(programID3, "lightColor");
	lightPos_id = glGetUniformLocation(programID3, "lightPos");

	//wczytanie tekstury
	tex_id = WczytajTeksture("objFiles/icelandic_mountain/ColorFx.bmp");
	if (tex_id == -1)
	{
		MessageBox(NULL, "Nie znaleziono pliku z teksturą", "Problem", MB_OK | MB_ICONERROR);
		exit(0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	uniformTex = glGetUniformLocation(programID3, "tex");
	glUniform1i(uniformTex, 0);

	glBindVertexArray(VAO[3]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //to samo VBO, ponieważ nadal chce wykorzystywac wierzchołki
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOnorm); //VBO wektorów normalnych
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOtex); //VBO tekstury
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);


	glutMainLoop();					
	
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &VBOnorm);
	glDeleteBuffers(1, &VBOcol);
	glDeleteBuffers(1, &VBOtex);
	glDeleteBuffers(4, VAO);
	return(0);
}

