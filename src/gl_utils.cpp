#include "gl_utils.h"



#define GL_LOG_FILE "log/gl.log"
#define MAX_SHADER_LENGTH 262144

/*---Registro---*/
bool restart_gl_log(){
	FILE *file = fopen(GL_LOG_FILE, "w");
	if(!file){
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE log file %s for writing\n",
			GL_LOG_FILE
		);
		return false;
	}
	time_t now = time(NULL);
	char *date = ctime(&now);
	fprintf(
		file,
		"GL_LOG_FILE log. local time %s\n",
		date
	);
	fclose(file);
	return true;
}

bool gl_log(const char *message, ...){
	va_list argptr;
	FILE *file = fopen(GL_LOG_FILE, "a");
	if(!file){
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
		);
		return false;
	}
	va_start(argptr, message);
	vfprintf(file, message, argptr);
	va_end(argptr);
	fclose(file);
	return true;
}

bool gl_log_err(const char *message, ...){
	va_list argptr;
	FILE *file = fopen(GL_LOG_FILE, "a");
	if(!file){
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
		);
		return false;
	}
	va_start(argptr, message);
	vfprintf(file, message, argptr);
	va_end(argptr);
	va_start(argptr, message);
	vfprintf(stderr, message, argptr);
	va_end(argptr);
	fclose(file);
	return true;
}

/*---GLFW3 y GLEW---*/
bool init(){
	
	restart_gl_log();

	gl_log("Starting GLFW %s", glfwGetVersionString());
	
	glfwSetErrorCallback(glfw_error_callback);
	if(!glfwInit()){
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	g_window = glfwCreateWindow(
		g_gl_width, g_gl_height, 
		"Batalien", 
		NULL, 
		NULL
	);

	if (!g_window){
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(g_window, glfw_window_size_callback);
	glfwMakeContextCurrent(g_window);
	glfwWindowHint(GLFW_SAMPLES, 4);
	
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); 
	glClearColor(0.2, 0.2, 0.2, 1.0); //BACKGROUND INICIAL: gris
	glViewport(0, 0, g_gl_width, g_gl_height);

	/* Version Info */
	const GLubyte* renderer = glGetString (GL_RENDERER);
	const GLubyte* version = glGetString (GL_VERSION); 
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);
	gl_log("Renderer: %s\nVersion: %s\n", renderer, version);
	
	return true;
}

void glfw_error_callback(int error, const char *description){
	fputs(description, stderr);
	gl_log_err("%s\n", description);
}

void glfw_window_size_callback(GLFWwindow *window, int width, int height){
	g_gl_width = width;
	g_gl_height = height;
	printf("Width %i - Height %i\n", width, height);
}

void updateFPS(GLFWwindow *window){
	static double previous_seconds = glfwGetTime();
	static int frame_count;
	double current_seconds = glfwGetTime();
	double elapsed_seconds = current_seconds - previous_seconds;
	if(elapsed_seconds > 0.25) {
		previous_seconds = current_seconds;
		double fps = (double)frame_count / elapsed_seconds;
		char tmp[128];
		sprintf (tmp, "Opengl @ fps: %.2f", fps);
		glfwSetWindowTitle(window, tmp);
		frame_count = 0;
	}
	frame_count++;
}

/*---Shaders---*/
bool parse_file_into_str(const char *file_name, char *shader_str, int max_len){
	shader_str[0] = '\0';
	FILE* file = fopen(file_name , "r");
	if(!file){
		gl_log_err("ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	int current_len = 0;
	char line[2048];
	strcpy(line, ""); // remember to clean up before using for first time!
	while(!feof(file)){
		if(NULL != fgets(line, 2048, file)){
			current_len += strlen(line);
			if (current_len >= max_len){
				gl_log_err(
					"ERROR: shader length is longer than string buffer length %i\n",
					max_len
				);
			}
			strcat (shader_str, line);
		}
	}
	if(EOF == fclose(file)){
		gl_log_err ("ERROR: closing file from reading %s\n", file_name);
		return false;
	}
	return true;
}

void log_shader_info(GLuint shader_index){
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetShaderInfoLog(shader_index, max_length, &actual_length, log);
	printf("Shader info log for GL index %i:\n%s\n", shader_index, log);
	gl_log("Shader info log for GL index %i:\n%s\n", shader_index, log);
}

bool create_shader (const char *file_name, GLuint *shader, GLenum type){
	gl_log("Creating shader from %s...\n", file_name);
	char shader_string[MAX_SHADER_LENGTH];
	parse_file_into_str(file_name, shader_string, MAX_SHADER_LENGTH);
	*shader = glCreateShader(type);
	const GLchar *p = (const GLchar*) shader_string;
	glShaderSource(*shader, 1, &p, NULL);
	glCompileShader(*shader);
	
	int params = -1;
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &params);
	if(GL_TRUE != params){
		gl_log_err("ERROR: GL shader index %i did not compile\n", *shader);
		log_shader_info(*shader);
		return false;
	}
	gl_log("Shader compiled. Index %i\n", *shader);
	return true;
}

void print_programme_info_log(GLuint sp){
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetProgramInfoLog(sp, max_length, &actual_length, log);
	printf("Program info log for GL index %u:\n%s", sp, log);
	gl_log("Program info log for GL index %u:\n%s", sp, log);
}

bool is_programme_valid (GLuint sp){
	glValidateProgram(sp);
	GLint params = -1;
	glGetProgramiv(sp, GL_VALIDATE_STATUS, &params);
	if(GL_TRUE != params){
		gl_log_err ("Program %i GL_VALIDATE_STATUS = GL_FALSE\n", sp);
		print_programme_info_log(sp);
		return false;
	}
	gl_log("Program %i GL_VALIDATE_STATUS = GL_TRUE\n", sp);
	return true;
}

bool create_programme(GLuint vert, GLuint frag, GLuint *programme){
	*programme = glCreateProgram();
	gl_log(
		"Created programme %u. Attaching shaders %u and %u...\n",
		*programme,
		vert,
		frag
	);
	glAttachShader(*programme, vert);
	glAttachShader(*programme, frag);
	glLinkProgram (*programme);

	GLint params = -1;
	glGetProgramiv(*programme, GL_LINK_STATUS, &params);
	if(GL_TRUE != params){
		gl_log_err(
			"ERROR: could not link shader programme GL index %u\n",
			*programme
		);
		print_programme_info_log(*programme);
		return false;
	}
	assert(is_programme_valid(*programme));
	glDeleteShader(vert);
	glDeleteShader(frag);
	return true;
}

GLuint create_programme_from_files(const char *vert_file_name, const char *frag_file_name){
	GLuint vert, frag, programme;
	assert(create_shader(vert_file_name, &vert, GL_VERTEX_SHADER));
	assert(create_shader(frag_file_name, &frag, GL_FRAGMENT_SHADER));
	assert(create_programme(vert, frag, &programme));
	return programme;
}
