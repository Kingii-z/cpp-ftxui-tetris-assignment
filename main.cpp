#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <vector>
#include <random>

using namespace ftxui;

const int WIDTH = 10;
const int HEIGHT = 20;

std::vector<std::vector<int>> shapes = {
	{{1,1,1,1}},
	{{1,1},{1,1}},
	{{0,1,1},{1,1,0}},
	{{1,1,0},{0,1,1}},
	{{1,1,1},{0,1,0}},
	{{1,0,0},{1,1,1}},
	{{0,0,1},{1,1,1}}
};

struct Block {
	std::vector<std::vector<int>> shape;
	int x, y;
};

Block current;
int board[HEIGHT][WIDTH] = {0};
int score = 0;
bool gameOver = false;

std::mt19937 rng{std::random_device{}()};

void newBlock() {
	std::uniform_int_distribution<int> dist(0, shapes.size()-1);
	current.shape = shapes[dist(rng)];
	current.x = WIDTH/2 - (int)current.shape[0].size()/2;
	current.y = 0;
	for(int i=0;i<current.shape.size();i++){
		for(int j=0;j<current.shape[i].size();j++){
			if(current.shape[i][j] && board[current.y+i][current.x+j]){
				gameOver = true;
			}
		}
	}
}

bool checkCollision(int dx, int dy, std::vector<std::vector<int>>& s) {
	for(int i=0;i<s.size();i++){
		for(int j=0;j<s[i].size();j++){
			if(s[i][j]){
				int nx = current.x + j + dx;
				int ny = current.y + i + dy;
				if(nx<0 || nx>=WIDTH || ny>=HEIGHT) return true;
				if(ny>=0 && board[ny][nx]) return true;
			}
		}
	}
	return false;
}

void merge() {
	for(int i=0;i<current.shape.size();i++){
		for(int j=0;j<current.shape[i].size();j++){
			if(current.shape[i][j]){
				board[current.y+i][current.x+j] = 1;
			}
		}
	}
}

void clearLines() {
	int lines = 0;
	for(int i=HEIGHT-1;i>=0;i--){
		bool full = true;
		for(int j=0;j<WIDTH;j++) if(!board[i][j]) full=false;
		if(full){
			lines++;
			for(int k=i;k>0;k--){
				for(int j=0;j<WIDTH;j++) board[k][j]=board[k-1][j];
			}
			for(int j=0;j<WIDTH;j++) board[0][j]=0;
			i++;
		}
	}
	score += lines*100;
}

std::vector<std::vector<int>> rotate(std::vector<std::vector<int>> s) {
	std::vector<std::vector<int>> res(s[0].size(), std::vector<int>(s.size()));
	for(int i=0;i<s.size();i++){
		for(int j=0;j<s[i].size();j++){
			res[j][s.size()-1-i] = s[i][j];
		}
	}
	return res;
}

int main() {
	newBlock();
	
	auto gameComponent = Renderer([&]{
		Element game;
		std::vector<Element> rows;
		
		int temp[HEIGHT][WIDTH];
		memcpy(temp, board, sizeof(board));
		if(!gameOver){
			for(int i=0;i<current.shape.size();i++){
				for(int j=0;j<current.shape[i].size();j++){
					if(current.shape[i][j]){
						int ty = current.y+i;
						int tx = current.x+j;
						if(ty>=0 && ty<HEIGHT && tx>=0 && tx<WIDTH){
							temp[ty][tx] = 2;
						}
					}
				}
			}
		}
		
		for(int i=0;i<HEIGHT;i++){
			std::vector<Element> row;
			for(int j=0;j<WIDTH;j++){
				if(temp[i][j]==0) row.push_back(text("  ") | bgcolor(Color::Black));
				if(temp[i][j]==1) row.push_back(text("  ") | bgcolor(Color::Blue));
				if(temp[i][j]==2) row.push_back(text("  ") | bgcolor(Color::Green));
			}
			rows.push_back(hbox(std::move(row)));
		}
		
		Element mainBoard = vbox(std::move(rows)) | border;
		
		Element info = vbox({
			text("俄罗斯方块") | bold | center,
			separator(),
			text("分数: " + std::to_string(score)),
			separator(),
			text("操作说明:"),
			text("← → 左右移动"),
			text("↑ 旋转方块"),
			text("↓ 加速下落"),
			gameOver ? text("游戏结束!") | color(Color::Red) : text("游戏进行中"),
		}) | border;
		
		return hbox(mainBoard, filler(), info);
	});
	
	gameComponent |= EventHandler([&](Event e){
		if(gameOver) return false;
		if(e == Event::ArrowLeft && !checkCollision(-1,0,current.shape)) current.x--;
		if(e == Event::ArrowRight && !checkCollision(1,0,current.shape)) current.x++;
		if(e == Event::ArrowDown && !checkCollision(0,1,current.shape)) current.y++;
		if(e == Event::ArrowUp){
			auto rotated = rotate(current.shape);
			if(!checkCollision(0,0,rotated)) current.shape = rotated;
		}
		return true;
	});
	while(true){
		auto screen = Screen::Create(Dimension::Full(), Dimension::Full());
		Render(screen, gameComponent->Render());
		screen.Print();
		if(!gameOver){
			if(!checkCollision(0,1,current.shape)){
				current.y++;
			}else{
				merge();
				clearLines();
				newBlock();
			}
		}
		usleep(350000);
	}
	return 0;
}
