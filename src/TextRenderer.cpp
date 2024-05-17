#include "TextRenderer.h"

TextRenderer::TextRenderer()  {
}

TextRenderer::~TextRenderer() {
}

void TextRenderer::loadFont(const std::string& path, unsigned int fontSize) {
	FT_Library lib;
	FT_Face face;

	FT_Error error = FT_Init_FreeType(&lib);

	if (error)
		printf("an error occuret duting fritype initializaton \n");

	error = FT_New_Face(lib, path.c_str(), 0, &face);

	if (error == FT_Err_Unknown_File_Format) {
		printf("font format is unsupported");
	}
	else if (error) {
		printf("font file couldn't be opened or read, or it is broken...");
	}

	FT_Set_Pixel_Sizes(face, 0, fontSize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);;

	for (GLubyte c = 0; c < 128; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			printf("ERROR::FREETYTPE: Failed to load Glyph \n");

		int height = face->glyph->bitmap.rows;

		Character character = {
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<unsigned int>(face->glyph->advance.x),
			m_fontAtlas.width
		};

		characters.insert(std::pair<char, Character>(c, character));

		//get highest char
		if (height > m_fontAtlas.height)
			m_fontAtlas.height = height;

		m_fontAtlas.width += face->glyph->bitmap.width;
	}

	unsigned char* map = (unsigned char*)calloc(m_fontAtlas.width * m_fontAtlas.height, 1);
	int offset = 0;
	for (GLubyte c = 0; c < 128; c++) {
		FT_Load_Char(face, c, FT_LOAD_RENDER);
		FT_Bitmap* bmp = &face->glyph->bitmap;

		int rows = bmp->rows;
		int width = bmp->width;

		for (int row = 0; row < rows; ++row) {
			for (int i = 0; i < width; ++i) {
				if (map) {
					map[row * int(m_fontAtlas.width) + offset + i] = bmp->buffer[row * bmp->pitch + i];
				}
			}
		}
		offset += width;
	}

	glGenTextures(1, &m_fontAtlas.id);
	glBindTexture(GL_TEXTURE_2D, m_fontAtlas.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_fontAtlas.width, m_fontAtlas.height, 0, GL_RED, GL_UNSIGNED_BYTE, map);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	FT_Done_Face(face);
	FT_Done_FreeType(lib);
}

void TextRenderer::init(GLFWwindow* window, int screenWidth, int screenHeight) {
	m_window = window;
	m_width = screenWidth;
	m_height = screenHeight;

	float w = 100.0;
	float h = 100.0;
	float xpos = 0.0f;
	float ypos = 0.0f;
	float vertices[] = {
		//pos xy            texture xy 
		xpos,     ypos + h, 0.0f, 0.0f, // top right
		xpos,     ypos,     0.0f, 0.0f, // bottom right
		w + xpos, ypos,     0.0f, 0.0f, // bottom left
		w + xpos, ypos + h, 0.0f, 0.0f // top left 
	};

	unsigned int indeces[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &m_EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indeces), indeces, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);

	glBindVertexArray(0);

	m_glyphShader = Shader("../assets/shaders/glyph");

	glUseProgram(m_glyphShader.id);
}

void TextRenderer::draw() {
	glm::vec2 position = glm::vec2(20.0f, 20.0f);
	auto color = glm::vec3(0.7f, 0.7f, 0.8f);
	drawText(m_text, position, color);
}

void TextRenderer::drawText(const std::string text, glm::vec2 position, glm::vec3 color) {
	position.y += scrollAmmount;
	m_textPosition = position;
	glm::vec2 charPos = position;
	for (auto c = text.begin(); c != text.end(); c++) {
		Character ch = characters[*c];
		float w = ch.size.x;
		float h = ch.size.y;

		float xpos = charPos.x + ch.bearing.x;
		float ypos = charPos.y + (this->characters['H'].bearing.y - ch.bearing.y);

		float texturePos = float(ch.texCoord) / float(m_fontAtlas.width);
		float texw = (float(ch.texCoord + w) / float(m_fontAtlas.width)) - texturePos;
		float texh = float(ch.size.y) / float(m_fontAtlas.height);

		TextureAtlasPart part = { texturePos, 0.0, texw, texh };

		if (*c != '\n') {
			glm::vec3 lineBgColor = { 0.2, 0.2, 0.2 };
			char c = 'h';
			Character ch = characters[c];

			float cw = ch.size.x;
			float chh = ch.size.y;

			drawQuadTexture(m_fontAtlas, { xpos, ypos }, w, h, part, color);
		}

		charPos.x += ch.advance >> 6;

		if (charPos.x + w >= m_width) {
			charPos.x = position.x;
			charPos.y += m_fontAtlas.height;
		}

		if (*c == '\n') {
			charPos.x = position.x;
			charPos.y += m_fontAtlas.height;
		}
	}

	auto carretPosition = getTextPositionFromIndex(m_carretIndex);

	int lines = getVisibleLinesCount();
	if (carretPosition.y >= m_height) {
		scrollAmmount = (lines - 2 - m_lineNumber) * m_fontAtlas.height;
	}

	if (carretPosition.y <= 0.0f) {
		scrollAmmount = m_lineNumber * -m_fontAtlas.height;
	}
}

void TextRenderer::drawQuadTexture(Texture tex, glm::vec2 position, float width, float height, TextureAtlasPart part, glm::vec3 color) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBindTexture(GL_TEXTURE_2D, m_fontAtlas.id);
	glUseProgram(m_glyphShader.id);

	float xpos = position.x;
	float ypos = position.y;

	float vertices[] = {
		// x     y
		 xpos,         ypos + height,    part.x,               part.y + part.height,
		 xpos,         ypos,             part.x,               part.y,
		 width + xpos, ypos,             part.x + part.width,  part.y,
		 width + xpos, ypos + height,    part.x + part.width,  part.y + part.height
	};

	unsigned int indices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glm::mat4 projection = glm::ortho(0.0f, (float)m_width, (float)m_height, 0.0f);
	glUniform3f(glGetUniformLocation(m_glyphShader.id, "textColor"), color.x, color.y, color.z);
	glUniformMatrix4fv(glGetUniformLocation(m_glyphShader.id, "projection"), 1, false, glm::value_ptr(projection));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
	glBindVertexArray(0);
}

void TextRenderer::debugDraw(const std::string& text) {
	m_text = text;
}

void TextRenderer::setText(const std::string& text) {
	m_text = text;
	calculateTextNewLineIndices();
}

int TextRenderer::getVisibleLinesCount() {
	return m_height / m_fontAtlas.height;
}

void TextRenderer::calculateTextNewLineIndices() {
	m_newLineIndices.clear();
	for (int i = 0; i < m_text.size(); i++) {
		if (m_text[i] == '\n') {
			m_newLineIndices.push_back(i);
		}
	}
}

glm::vec2 TextRenderer::getTextPositionFromIndex(int textIndex) {
	int index = 0;
	glm::vec2 carretPosition = m_textPosition;
	for (auto c = m_text.begin(); c != m_text.end(); c++) {
		if (index >= textIndex) {
			break;
		}

		Character ch = characters[*c];
		float w = ch.size.x;
		float h = ch.size.y;

		carretPosition.x += ch.advance >> 6;

		if (carretPosition.x + w >= m_width) {
			carretPosition.x = m_textPosition.x;
			carretPosition.y += m_fontAtlas.height;
		}

		if (*c == '\n') {
			carretPosition.x = m_textPosition.x;
			carretPosition.y += m_fontAtlas.height;
		}

		index++;
	}
	return carretPosition;
}

void TextRenderer::setLineNumber(int lineNumber) {
	m_lineNumber = lineNumber;
}