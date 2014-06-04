//
//  MainScene.cpp
//  KawazCatch
//
//  Created by giginet on 5/15/14.
//
//

#include "MainScene.h"
#include "TitleScene.h"
#include <random>
#include "SimpleAudioEngine.h"

USING_NS_CC;

/// マージン
const int FRUIT_TOP_MERGIN = 40;
/// 制限時間
const float TIME_LIMIT_SECOND = 60;
/// 落下速度
const float FALLING_DURATION = 3.0;

Scene* Main::createScene()
{
    auto scene = Scene::create();
    auto layer = Main::create();
    scene->addChild(layer);
    return scene;
}

bool Main::init()
{
    if (!Layer::init()) {
        return false;
    }
    
    // 初期スコアの設定
    _score = 0;
    
    // クラッシュ状態の設定
    _isCrash = false;
    
    // 乱数周りの初期化
    _lot = 0;
    std::random_device rdev;
    _engine.seed(rdev());
    
    
    // 背景を表示する
    auto director = Director::getInstance();
    auto size = director->getWinSize();
    auto background = Sprite::create("background.png");
    background->setPosition(Vec2(size.width / 2.0, size.height / 2.0));
    this->addChild(background);
    
    // プレイヤーを表示する
    this->setPlayer(Sprite::create("player.png"));
    _player->setPosition(Vec2(size.width / 2.0, 220));
    this->addChild(_player);
    
    // イベントリスナーの追加
    auto listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = [](Touch* touch, Event* event) {
        // タッチされたとき
        return true;
    };
    listener->onTouchMoved = [this, size](Touch* touch, Event* event) {
        // タッチ位置が動いたとき
        if (!this->getIsCrash()) { // クラッシュしてないとき
            Vec2 delta = touch->getDelta(); // 前回とのタッチ位置との差をベクトルで取得する
            Vec2 position = _player->getPosition(); // 現在のかわずたんの座標を取得する
            Vec2 newPosition = position + delta;
            newPosition = newPosition.getClampPoint(Vec2(0, position.y), Vec2(size.width, position.y));
            _player->setPosition(newPosition); // 現在座標 + 移動量を新たな座標にする
        }
    };
    this->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);
    
    // スコアラベルの追加
    auto scoreLabel = Label::createWithSystemFont(std::to_string(_score), "Helvetica", 64);
    scoreLabel->enableShadow();
    scoreLabel->enableOutline(Color4B::RED, 5);
    this->setScoreLabel(scoreLabel);
    _scoreLabel->setPosition(Vec2(size.width / 2.0 * 1.5, size.height - 60));
    this->addChild(_scoreLabel);
    
    // タイマーラベルの追加
    _second = TIME_LIMIT_SECOND;
    auto secondLabel = Label::createWithSystemFont(std::to_string((int)_second), "Helvetica", 64);
    secondLabel->enableShadow();
    secondLabel->enableOutline(Color4B::RED, 5);
    secondLabel->setPosition(Vec2(size.width / 2.0, size.height - 60));
    this->setSecondLabel(secondLabel);
    this->addChild(_secondLabel);
    this->scheduleUpdate();
    
    // 初期状態をREADYにする
    _state = GameState::READY;
    
    return true;
}

Main::~Main()
{
    // デストラクタ
    CC_SAFE_RELEASE_NULL(_player);
    CC_SAFE_RELEASE_NULL(_scoreLabel);
    CC_SAFE_RELEASE_NULL(_secondLabel);
}

void Main::onEnterTransitionDidFinish()
{
    // シーン遷移が完了したとき
    Layer::onEnterTransitionDidFinish();
    
    // BGMを鳴らす
    CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic("main.mp3", true);
    
    this->addReadyLabel();
}

void Main::update(float dt)
{
    if (_state == GameState::PLAYING) {
        // PLAYING状態の時
        if (_second > FALLING_DURATION && _lot == 0) {
            
            
            float t = _second / 1.5f;
            t = MAX(t, 12);
            std::binomial_distribution<> dest(t, 0.5);
            _lot = dest(_engine);
            this->addFruit();
        } else {
            --_lot;
        }
        
        for (auto fruit : _fruits) {
            auto busketPosition = _player->getPosition() - Vec2(0, 10);
            bool isHit = fruit->getBoundingBox().containsPoint(busketPosition);
            if (isHit) {
                this->catchFruit(fruit);
            }
        }
        _second -= dt;
        _secondLabel->setString(std::to_string((int)_second));
        if (_second < 0) {
            _state = GameState::ENDING;
            // 終了文字の表示
            auto finish = Sprite::create("finish.png");
            auto winSize = Director::getInstance()->getWinSize();
            finish->setPosition(Vec2(winSize.width / 2.0, winSize.height / 2.0));
            finish->setScale(0);
            auto appear = EaseExponentialIn::create(ScaleTo::create(0.25, 1.0));
            auto disappear = EaseExponentialIn::create(ScaleTo::create(0.25, 0));
            
            finish->runAction(Sequence::create(appear,
                                               DelayTime::create(2.0),
                                               disappear,
                                               DelayTime::create(1.0),
                                               CallFunc::create([this] {
                _state = GameState::RESULT;
                this->addResultMenu();
            }),
                                               NULL));
            this->addChild(finish);
        }
    }
}

Sprite* Main::addFruit()
{
    
    auto winSize = Director::getInstance()->getWinSize();
    float p = _second < 20 ? 12 : 5;
    int fruitType = 0;
    int r = this->generateRandom(100);
    if (r <= p) {
        fruitType = (int)FruitType::GOLDEN;
    } else if (r <= p * 2) {
        fruitType = (int)FruitType::BOMB;
    } else {
        fruitType = this->generateRandom(4);
    }
    
    std::string filename = "fruit" + std::to_string(fruitType) + ".png";
    auto fruit = Sprite::create(filename);
    fruit->setTag(fruitType);
    
    auto fruitSize = fruit->getContentSize();
    float min = fruitSize.width / 2.0;
    float max = winSize.width - fruitSize.width / 2.0;
    std::uniform_int_distribution<float> posDist(min, max);
    float fruitXPos = posDist(_engine);
    
    fruit->setPosition(Vec2(fruitXPos,
                             winSize.height - FRUIT_TOP_MERGIN - fruitSize.height / 2.0));
    this->addChild(fruit);
    _fruits.pushBack(fruit);
    
    auto ground = Vec2(fruitXPos, 0);
    fruit->setScale(0);
    fruit->runAction(Sequence::create(ScaleTo::create(0.25, 1),
                                      Repeat::create(Sequence::create(RotateTo::create(0.25, -30), RotateTo::create(0.25, 30), NULL), 2),
                                      RotateTo::create(0, 0.125),
                                      MoveTo::create(FALLING_DURATION, ground),
                                      RemoveSelf::create(),
                                      NULL));
    return fruit;
}

void Main::catchFruit(cocos2d::Sprite *fruit)
{
    FruitType fruitType = (FruitType)fruit->getTag();
    fruit->removeFromParent();
    _fruits.eraseObject(fruit);
    switch (fruitType) {
        case Main::FruitType::GOLDEN:
            _score += 5;
            break;
        case Main::FruitType::BOMB:
            this->onCatchBomb();
            break;
        default:
            _score += 1;
            break;
    }
    _scoreLabel->setString(std::to_string(_score));
}

void Main::addReadyLabel()
{
    auto winSize = Director::getInstance()->getWinSize();
    auto center = Vec2(winSize.width / 2.0, winSize.height / 2.0);
    auto ready = Sprite::create("ready.png");
    ready->setScale(0);
    ready->setPosition(center);
    this->addChild(ready);
    auto start = Sprite::create("start.png");
    start->runAction(Sequence::create(CCSpawn::create(EaseIn::create(ScaleTo::create(0.5, 5.0), 0.5),
                                                      FadeOut::create(0.5),
                                                      NULL),
                                      RemoveSelf::create(), NULL));
    start->setPosition(center);
    ready->runAction(Sequence::create(ScaleTo::create(0.25, 1),
                                      DelayTime::create(1.0),
                                      CallFunc::create([this, start] {
        this->addChild(start);
        _state = GameState::PLAYING;
    }),
                                      RemoveSelf::create(),
                                      NULL));
    
}

void Main::addResultMenu()
{
    // ENDING状態のとき
    _state = GameState::RESULT;
    auto winSize = Director::getInstance()->getWinSize();
    // 「もう一度遊ぶ」ボタン
    auto replayButton = MenuItemImage::create("replay_button.png",
                                              "replay_button_pressed.png",
                                              [this](Ref* ref) {
                                                  // 「もう一度遊ぶ」ボタンを押したときの処理
                                                  auto scene = Main::createScene();
                                                  auto transition = TransitionFade::create(0.5, scene);
                                                  Director::getInstance()->replaceScene(transition);
                                              });
    // 「タイトルへ戻る」ボタン
    auto titleButton = MenuItemImage::create("title_button.png",
                                             "title_button_pressed.png",
                                             [this](Ref* ref) {
                                                 // 「タイトルへ戻る」ボタンを押したときの処理
                                                 auto scene = Title::createScene();
                                                 auto transition = TransitionCrossFade::create(0.5, scene);
                                                 Director::getInstance()->replaceScene(transition);
                                             });
    auto menu = Menu::create(replayButton, titleButton, NULL);
    menu->alignItemsVerticallyWithPadding(30);
    menu->setPosition(Vec2(winSize.width / 2.0, winSize.height / 2.0));
    this->addChild(menu);
}

void Main::onCatchBomb()
{
    _isCrash = true; // クラッシュ状態
    // ToDo アニメーション
    _player->runAction(Sequence::create(DelayTime::create(3.0),
                                        CallFunc::create([this] {
        _isCrash = false;
    }),
                                        NULL));
    _score = MAX(0, _score - 4); // 0未満になったら0点にする
}

int Main::generateRandom(int n)
{
    std::uniform_int_distribution<> dist(0, n);
    return dist(_engine);
}