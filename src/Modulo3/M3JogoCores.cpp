/* Jogo das Cores - Implementação Completa
 * Adaptado do código original de Rossana Baptista Queiroz
 * 
 * Regras do jogo:
 * - Clique em um quadrado para remover todos os quadrados com cores similares
 * - Pontuação inicial: 1000 pontos
 * - A cada tentativa, a pontuação máxima diminui em 50 pontos
 * - Pontos ganhos = (quadrados removidos) * (pontuação atual)
 * - Pressione R para reiniciar o jogo
 * - Pressione ESC para sair
 */

 #include <iostream>
 #include <string>
 #include <assert.h>
 #include <vector>
 #include <sstream>
 #include <iomanip>
 
 using namespace std;
 
 // GLAD
 #include <glad/glad.h>
 
 // GLFW
 #include <GLFW/glfw3.h>
 
 // GLM
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/type_ptr.hpp>
 
 using namespace glm;
 
 #include <cmath>
 #include <ctime>
 
 // Protótipos das funções
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
 void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
 GLuint createQuad();
 int setupShader();
 void eliminarSimilares(float tolerancia);
 void initializeGrid();
 void resetGame();
 int contarQuadradosRestantes();
 
 // Dimensões da janela
 const GLuint WIDTH = 800, HEIGHT = 600;
 const GLuint ROWS = 6, COLS = 8;
 const GLuint QUAD_WIDTH = 100, QUAD_HEIGHT = 100;
 const float dMax = sqrt(3.0);
 
 // Variáveis de pontuação
 int pontuacaoTotal = 0;
 int pontuacaoAtual = 1000;
 int tentativas = 0;
 const int REDUCAO_PONTUACAO = 50;
 bool jogoTerminado = false;
 
 // Shaders
 const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 uniform mat4 projection;
 uniform mat4 model;
 void main()	
 {
	 gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
 }
 )";
 
 const GLchar *fragmentShaderSource = R"(
 #version 400
 uniform vec4 inputColor;
 out vec4 color;
 void main()
 {
	 color = inputColor;
 }
 )";
 
 struct Quad
 {
	 vec3 position;
	 vec3 dimensions;
	 vec3 color;
	 bool eliminated;
 };
 
 // Grid de quadrados
 Quad grid[ROWS][COLS];
 int iSelected = -1;
 
 // Função para inicializar a grid
 void initializeGrid()
 {
	 srand(time(0));
	 
	 for (int i = 0; i < ROWS; i++)
	 {
		 for (int j = 0; j < COLS; j++)
		 {
			 Quad quad;
			 vec2 ini_pos = vec2(QUAD_WIDTH / 2, QUAD_HEIGHT / 2);
			 quad.position = vec3(ini_pos.x + j * QUAD_WIDTH, ini_pos.y + i * QUAD_HEIGHT, 0.0);
			 quad.dimensions = vec3(QUAD_WIDTH, QUAD_HEIGHT, 1.0);
			 
			 // Gerar cores aleatórias
			 float r = (rand() % 256) / 255.0f;
			 float g = (rand() % 256) / 255.0f;
			 float b = (rand() % 256) / 255.0f;
			 quad.color = vec3(r, g, b);
			 quad.eliminated = false;
			 
			 grid[i][j] = quad;
		 }
	 }
 }
 
 // Função para resetar o jogo
 void resetGame()
 {
	 pontuacaoTotal = 0;
	 pontuacaoAtual = 1000;
	 tentativas = 0;
	 jogoTerminado = false;
	 iSelected = -1;
	 
	 initializeGrid();
	 
	 cout << "\n=== JOGO REINICIADO ===" << endl;
	 cout << "Pontuacao inicial: " << pontuacaoAtual << " pontos" << endl;
	 cout << "Pressione R para reiniciar ou ESC para sair" << endl;
 }
 
 // Função para contar quadrados restantes
 int contarQuadradosRestantes()
 {
	 int count = 0;
	 for (int i = 0; i < ROWS; i++)
	 {
		 for (int j = 0; j < COLS; j++)
		 {
			 if (!grid[i][j].eliminated)
				 count++;
		 }
	 }
	 return count;
 }
 
 // Função MAIN
 int main()
 {
	 // Inicialização da GLFW
	 glfwInit();
 
	 // Criação da janela GLFW
	 GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das Cores - Clique para eliminar cores similares!", nullptr, nullptr);
	 glfwMakeContextCurrent(window);
 
	 // Registro das funções de callback
	 glfwSetKeyCallback(window, key_callback);
	 glfwSetMouseButtonCallback(window, mouse_button_callback);
 
	 // GLAD: carrega todos os ponteiros de funções da OpenGL
	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	 {
		 std::cout << "Failed to initialize GLAD" << std::endl;
		 return -1;
	 }
 
	 // Informações de versão
	 const GLubyte *renderer = glGetString(GL_RENDERER);
	 const GLubyte *version = glGetString(GL_VERSION);
	 cout << "Renderer: " << renderer << endl;
	 cout << "OpenGL version supported " << version << endl;
 
	 // Configuração da viewport
	 int width, height;
	 glfwGetFramebufferSize(window, &width, &height);
	 glViewport(0, 0, width, height);
 
	 // Setup dos shaders e geometria
	 GLuint shaderID = setupShader();
	 GLuint VAO = createQuad();
 
	 // Inicializar o jogo
	 resetGame();
 
	 glUseProgram(shaderID);
 
	 // Localização das variáveis uniform
	 GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
 
	 // Matriz de projeção ortográfica
	 mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	 glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
 
	 // Loop principal do jogo
	 while (!glfwWindowShouldClose(window))
	 {
		 glfwPollEvents();
 
		 // Limpa o buffer de cor
		 glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		 glClear(GL_COLOR_BUFFER_BIT);
 
		 glBindVertexArray(VAO);
 
		 // Processar seleção se houver
		 if (iSelected > -1)
		 {
			 eliminarSimilares(0.3f); // Tolerância ajustada para melhor jogabilidade
		 }
 
		 // Renderizar quadrados não eliminados
		 for (int i = 0; i < ROWS; i++)
		 {
			 for (int j = 0; j < COLS; j++)
			 {
				 if (!grid[i][j].eliminated)
				 {
					 mat4 model = mat4(1);
					 model = translate(model, grid[i][j].position);
					 model = scale(model, grid[i][j].dimensions);
					 
					 glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
					 glUniform4f(colorLoc, grid[i][j].color.r, grid[i][j].color.g, grid[i][j].color.b, 1.0f);
					 
					 glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				 }
			 }
		 }
 
		 glBindVertexArray(0);
 
		 // Verificar condição de vitória
		 int quadradosRestantes = contarQuadradosRestantes();
		 if (quadradosRestantes == 0 && !jogoTerminado)
		 {
			 jogoTerminado = true;
			 cout << "\n🎉 PARABENS! VOCE VENCEU! 🎉" << endl;
			 cout << "Pontuacao final: " << pontuacaoTotal << " pontos" << endl;
			 cout << "Tentativas: " << tentativas << endl;
			 cout << "Pressione R para jogar novamente!" << endl;
		 }
 
		 glfwSwapBuffers(window);
	 }
 
	 // Limpeza
	 glDeleteVertexArrays(1, &VAO);
	 glfwTerminate();
	 return 0;
 }
 
 // Callback de teclado
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
 {
	 if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		 glfwSetWindowShouldClose(window, GL_TRUE);
	 
	 if (key == GLFW_KEY_R && action == GLFW_PRESS)
		 resetGame();
 }
 
 // Setup dos shaders
 int setupShader()
 {
	 // Vertex shader
	 GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	 glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	 glCompileShader(vertexShader);
	 
	 GLint success;
	 GLchar infoLog[512];
	 glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	 }
 
	 // Fragment shader
	 GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	 glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	 glCompileShader(fragmentShader);
	 
	 glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	 if (!success)
	 {
		 glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	 }
 
	 // Programa de shader
	 GLuint shaderProgram = glCreateProgram();
	 glAttachShader(shaderProgram, vertexShader);
	 glAttachShader(shaderProgram, fragmentShader);
	 glLinkProgram(shaderProgram);
	 
	 glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	 if (!success)
	 {
		 glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		 std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	 }
	 
	 glDeleteShader(vertexShader);
	 glDeleteShader(fragmentShader);
 
	 return shaderProgram;
 }
 
 // Callback do mouse
 void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
 {
	 if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !jogoTerminado)
	 {
		 double xpos, ypos;
		 glfwGetCursorPos(window, &xpos, &ypos);
		 
		 int x = (int)(xpos / QUAD_WIDTH);
		 int y = (int)(ypos / QUAD_HEIGHT);
		 
		 // Verificar limites
		 if (x >= 0 && x < COLS && y >= 0 && y < ROWS && !grid[y][x].eliminated)
		 {
			 cout << "\nTentativa " << (tentativas + 1) << ":" << endl;
			 cout << "Posicao clicada: (" << x << ", " << y << ")" << endl;
			 cout << "Cor RGB: (" << fixed << setprecision(2) 
				  << grid[y][x].color.r << ", " 
				  << grid[y][x].color.g << ", " 
				  << grid[y][x].color.b << ")" << endl;
			 
			 iSelected = x + y * COLS;
			 tentativas++;
		 }
	 }
 }
 
 // Criação do VAO para o quadrado
 GLuint createQuad()
 {
	 GLuint VAO;
 
	 GLfloat vertices[] = {
		 // Triângulo strip para formar um quadrado
		 -0.5f,  0.5f, 0.0f,  // v0 (top-left)
		 -0.5f, -0.5f, 0.0f,  // v1 (bottom-left)
		  0.5f,  0.5f, 0.0f,  // v2 (top-right)
		  0.5f, -0.5f, 0.0f   // v3 (bottom-right)
	 };
 
	 GLuint VBO;
	 glGenBuffers(1, &VBO);
	 glBindBuffer(GL_ARRAY_BUFFER, VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
 
	 glGenVertexArrays(1, &VAO);
	 glBindVertexArray(VAO);
	 
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	 glEnableVertexAttribArray(0);
 
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);
 
	 return VAO;
 }
 
 // Função para eliminar cores similares
 void eliminarSimilares(float tolerancia)
 {
	 int x = iSelected % COLS;
	 int y = iSelected / COLS;
	 
	 vec3 corSelecionada = grid[y][x].color;
	 int quadradosEliminados = 0;
	 
	 // Marcar quadrados similares para eliminação
	 for (int i = 0; i < ROWS; i++)
	 {
		 for (int j = 0; j < COLS; j++)
		 {
			 if (!grid[i][j].eliminated)
			 {
				 vec3 corAtual = grid[i][j].color;
				 
				 // Calcular distância euclidiana no espaço RGB
				 float distancia = sqrt(
					 pow(corSelecionada.r - corAtual.r, 2) + 
					 pow(corSelecionada.g - corAtual.g, 2) + 
					 pow(corSelecionada.b - corAtual.b, 2)
				 );
				 
				 // Normalizar pela distância máxima possível
				 float distanciaNormalizada = distancia / dMax;
				 
				 if (distanciaNormalizada <= tolerancia)
				 {
					 grid[i][j].eliminated = true;
					 quadradosEliminados++;
				 }
			 }
		 }
	 }
	 
	 // Calcular pontuação
	 int pontosGanhos = quadradosEliminados * pontuacaoAtual;
	 pontuacaoTotal += pontosGanhos;
	 
	 // Exibir resultados da tentativa
	 cout << "Quadrados eliminados: " << quadradosEliminados << endl;
	 cout << "Pontos ganhos: " << pontosGanhos << " (" << quadradosEliminados 
		  << " x " << pontuacaoAtual << ")" << endl;
	 cout << "Pontuacao total: " << pontuacaoTotal << endl;
	 
	 // Reduzir pontuação para próxima tentativa
	 pontuacaoAtual = std::max(50, pontuacaoAtual - REDUCAO_PONTUACAO);
	 cout << "Proxima tentativa vale: " << pontuacaoAtual << " pontos cada quadrado" << endl;
	 
	 // Mostrar progresso
	 int restantes = contarQuadradosRestantes();
	 cout << "Quadrados restantes: " << restantes << " de " << (ROWS * COLS) << endl;
	 
	 iSelected = -1;
 }