//
//  MainScene.cpp
//  KawazCatch
//
//  Created by giginet on 5/15/14.
//
//

#include "MainScene.h"
#include <random>

USING_NS_CC;

/// マージン
const int FRUIT_TOP_MERGIN = 40;
/// 制限時間
const float TIME_LIMIT_SECOND = 5;

Scene* MainScene::createScene()
{
    auto scene = Scene::create();
    auto layer = MainScene::create();
    scene->addChild(layer);
    return scene;
}

bool MainScene::init()
{
    if (!Layer::init()) {
        return false;
    }
    
    _score = 0;
    
    auto director = Director::getInstance();
    auto size = director->getWinSize();
    auto background = Sprite::create("background.png");
    background->setPosition(Point(size.width / 2.0, size.height / 2.0));
    this->addChild(background);
    
    this->setPlayer(Sprite::create("player.png"));
    _player->setPosition(Point(size.width / 2.0, 220));
    this->addChild(_player);
    
    auto listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = [](Touch* touch, Event* event) {
        log("Touch at (%f, %f)", touch->getLocation().x, touch->getLocation().y);
        return true;
    };
    listener->onTouchMoved = [this, size](Touch* touch, Event* event) {
        Point delta = touch->getDelta(); // 前回とのタッチ位置との差をベクトルで取得する
        Point position = _player->getPosition(); // 現在のかわずたんの座標を取得する
        Point newPosition = position + delta;
        newPosition = newPosition.getClampPoint(Point(0, position.y), Point(size.width, position.y));
        _player->setPosition(newPosition); // 現在座標 + 移動量を新たな座標にする
    };
    director->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);
    
    // スコアラベルの追加
    auto scoreLabel = Label::createWithSystemFont(std::to_string(_score), "Helvetica", 64);
    scoreLabel->enableShadow();
    scoreLabel->enableOutline(Color4B::RED, 5);
    this->setScoreLabel(scoreLabel);
    _scoreLabel->setPosition(Point(size.width / 2.0 * 1.5, size.height - 60));
    this->addChild(_scoreLabel);
    
    // タイマーラベルの追加
    _second = TIME_LIMIT_SECOND;
    auto secondLabel = Label::createWithSystemFont(std::to_string((int)_second), "Helvetica", 64);
    secondLabel->enableShadow();
    secondLabel->enableOutline(Color4B::RED, 5);
    secondLabel->setPosition(Point(size.width / 2.0, size.height - 60));
    this->setSecondLabel(secondLabel);
    this->addChild(_secondLabel);
    this->scheduleUpdate();
    
    _lot = 0;
    std::random_device rdev;
    _engine.seed(rdev());
    
    _state = GameState::PLAYING;
    
    return true;
}

MainScene::~MainScene()
{
    CC_SAFE_RELEASE_NULL(_player);
    CC_SAFE_RELEASE_NULL(_scoreLabel);
    CC_SAFE_RELEASE_NULL(_secondLabel);
}

void MainScene::update(float dt)
{
    if (_state == GameState::PLAYING) {
        if (_second > 0 && _lot == 0) {
            std::binomial_distribution<> dest(_second * 2.0, 0.5);
            _lot = dest(_engine);
            this->addFruit();
        } else {
            --_lot;
        }
        
        for (auto fruit : _fruits) {
            auto busketPosition = _player->getPosition() - Point(0, 10);
            bool isHit = fruit->getBoundingBox().containsPoint(busketPosition);
            if (isHit) {
                this->catchFruit(fruit);
            }
        }
        _second -= dt;
        _secondLabel->setString(std::to_string((int)_second));
        if (_second < 0) {
            _state = GameState::RESULT;
        }
    } else if (_state == GameState::RESULT) {
        auto winSize = Director::getInstance()->getWinSize();
        auto replayButton = MenuItemImage::create("replay_button.png",
                                                  "replay_button_pressed.png",
                                                  [this](Ref* ref) {
                                                      // ボタンを押したときの処理
                                                      auto scene = MainScene::createScene();
                                                      Director::getInstance()->replaceScene(scene);
                                                  });
        auto titleButton = MenuItemImage::create("replay_button.png",
                                                 "replay_button_pressed.png",
                                                  [this](Ref* ref) {
                                                      // ボタンを押したときの処理
                                                  });
        auto menu = Menu::create(replayButton, titleButton, NULL);
        menu->alignItemsVerticallyWithPadding(30);
        menu->setPosition(Point(winSize.width / 2.0, winSize.height / 2.0));
        this->addChild(menu);
    }
}

Sprite* MainScene::addFruit()
{
    std::uniform_int_distribution<> dist(0, (int)FruitType::COUNT - 1);
    
    auto winSize = Director::getInstance()->getWinSize();
    int fruitNumber = dist(_engine);
    
    std::string filename = "fruit" + std::to_string(fruitNumber) + ".png";
    auto fruit = Sprite::create(filename);
    
    auto fruitSize = fruit->getContentSize();
    float min = fruitSize.width / 2.0;
    float max = winSize.width - fruitSize.width / 2.0;
    std::uniform_int_distribution<float> posDist(min, max);
    float fruitXPos = posDist(_engine);
    
    fruit->setPosition(Point(fruitXPos, winSize.height - FRUIT_TOP_MERGIN - fruitSize.height / 2.0));
    this->addChild(fruit);
    _fruits.pushBack(fruit);
    
    auto ground = Point(fruitXPos, 0);
    fruit->runAction(Sequence::create(MoveTo::create(3.0, ground),
                                      RemoveSelf::create(),
                                      NULL));
    return fruit;
}

void MainScene::catchFruit(cocos2d::Sprite *fruit)
{
    fruit->removeFromParent();
    _fruits.eraseObject(fruit);
    _score += 1;
    _scoreLabel->setString(std::to_string(_score));
}