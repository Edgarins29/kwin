/**************************************************************************
 * KWin - the KDE window manager                                          *
 * This file is part of the KDE project.                                  *
 *                                                                        *
 * Copyright (C) 2013 Antonis Tsiapaliokas <kok3rs@gmail.com>             *
 * Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>                 *
 *                                                                        *
 * This program is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/


#include "compositing.h"
#include "model.h"
#include "ui_compositing.h"
#include <QAction>
#include <QApplication>
#include <QLayout>

#include <kcmodule.h>
#include <kservice.h>

class KWinCompositingKCM : public KCModule
{
    Q_OBJECT
public:
    virtual ~KWinCompositingKCM();

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

protected:
    explicit KWinCompositingKCM(QWidget* parent, const QVariantList& args,
                                KWin::Compositing::EffectView::ViewType viewType);

private:
    QScopedPointer<KWin::Compositing::EffectView> m_view;
};

class KWinDesktopEffects : public KWinCompositingKCM
{
    Q_OBJECT
public:
    explicit KWinDesktopEffects(QWidget* parent = 0, const QVariantList& args = QVariantList())
        : KWinCompositingKCM(parent, args, KWin::Compositing::EffectView::DesktopEffectsView) {}
};

class KWinCompositingSettings : public KCModule
{
    Q_OBJECT
public:
    explicit KWinCompositingSettings(QWidget *parent = 0, const QVariantList &args = QVariantList());

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

private:
    void init();
    KWin::Compositing::Compositing *m_compositing;
    Ui_CompositingForm m_form;
};

KWinCompositingSettings::KWinCompositingSettings(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
    , m_compositing(new KWin::Compositing::Compositing(this))
{
    m_form.setupUi(this);
    m_form.glCrashedWarning->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
    QAction *reenableGLAction = new QAction(i18n("Re-enable OpenGL detection"), this);
    connect(reenableGLAction, &QAction::triggered, m_compositing, &KWin::Compositing::Compositing::reenableOpenGLDetection);
    connect(reenableGLAction, &QAction::triggered, m_form.glCrashedWarning, &KMessageWidget::animatedHide);
    m_form.glCrashedWarning->addAction(reenableGLAction);
    m_form.scaleWarning->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
    m_form.tearingWarning->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
    m_form.windowThumbnailWarning->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));

    m_form.compositingEnabled->setVisible(!m_compositing->compositingRequired());
    m_form.windowsBlockCompositing->setVisible(!m_compositing->compositingRequired());

    init();
}

void KWinCompositingSettings::init()
{
    using namespace KWin::Compositing;
    auto currentIndexChangedSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    connect(m_compositing, &Compositing::changed, this, static_cast<void(KCModule::*)()>(&KWinCompositingSettings::changed));

    // enabled check box
    m_form.compositingEnabled->setChecked(m_compositing->compositingEnabled());
    connect(m_compositing, &Compositing::compositingEnabledChanged, m_form.compositingEnabled, &QCheckBox::setChecked);
    connect(m_form.compositingEnabled, &QCheckBox::toggled, m_compositing, &Compositing::setCompositingEnabled);

    // animation speed
    m_form.animationSpeed->setValue(m_compositing->animationSpeed());
    connect(m_compositing, &Compositing::animationSpeedChanged, m_form.animationSpeed, &QSlider::setValue);
    connect(m_form.animationSpeed, &QSlider::valueChanged, m_compositing, &Compositing::setAnimationSpeed);

    // gl scale filter
    m_form.glScaleFilter->setCurrentIndex(m_compositing->glScaleFilter());
    connect(m_compositing, &Compositing::glScaleFilterChanged, m_form.glScaleFilter, &QComboBox::setCurrentIndex);
    connect(m_form.glScaleFilter, currentIndexChangedSignal, m_compositing, &Compositing::setGlScaleFilter);
    connect(m_form.glScaleFilter, currentIndexChangedSignal,
        [this](int index) {
            if (index == 2) {
                m_form.scaleWarning->animatedShow();
            } else {
                m_form.scaleWarning->animatedHide();
            }
        }
    );

    // xrender scale filter
    m_form.xrScaleFilter->setCurrentIndex(m_compositing->xrScaleFilter());
    connect(m_compositing, &Compositing::xrScaleFilterChanged, m_form.xrScaleFilter, &QComboBox::setCurrentIndex);
    connect(m_form.xrScaleFilter, currentIndexChangedSignal,
            [this](int index) {
        if (index == 0) {
            m_compositing->setXrScaleFilter(false);
        } else {
            m_compositing->setXrScaleFilter(true);
        }
    });

    // tearing prevention
    m_form.tearingPrevention->setCurrentIndex(m_compositing->glSwapStrategy());
    connect(m_compositing, &Compositing::glSwapStrategyChanged, m_form.tearingPrevention, &QComboBox::setCurrentIndex);
    connect(m_form.tearingPrevention, currentIndexChangedSignal, m_compositing, &Compositing::setGlSwapStrategy);
    connect(m_form.tearingPrevention, currentIndexChangedSignal,
        [this](int index) {
            if (index == 2) {
                // only when cheap - tearing
                m_form.tearingWarning->setText(i18n("\"Only when cheap\" only prevents tearing for full screen changes like a video."));
                m_form.tearingWarning->animatedShow();
            } else if (index == 3) {
                // full screen repaints
                m_form.tearingWarning->setText(i18n("\"Full screen repaints\" can cause performance problems."));
                m_form.tearingWarning->animatedShow();
            } else if (index == 4) {
                // re-use screen content
                m_form.tearingWarning->setText(i18n("\"Re-use screen content\" causes severe performance problems on MESA drivers."));
                m_form.tearingWarning->animatedShow();
            } else {
                m_form.tearingWarning->animatedHide();
            }
        }
    );

    // Vulkan device
    if (auto *model = m_compositing->vulkanDeviceModel()) {
        m_form.vulkanDevice->setModel((QAbstractItemModel *) model);
        m_form.vulkanDevice->setCurrentIndex(m_compositing->vkDevice());
    }
    connect(m_compositing, &Compositing::vkDeviceChanged, m_form.vulkanDevice, &QComboBox::setCurrentIndex);
    connect(m_form.vulkanDevice, currentIndexChangedSignal, m_compositing, &Compositing::setVkDevice);

    // Vulkan V-sync setting
    m_form.vulkanVSync->setCurrentIndex(m_compositing->vkVSync());
    connect(m_compositing, &Compositing::vkVSyncChanged, m_form.vulkanVSync, &QComboBox::setCurrentIndex);
    connect(m_form.vulkanVSync, currentIndexChangedSignal, m_compositing, &Compositing::setVkVSync);

    // windowThumbnail
    m_form.windowThumbnail->setCurrentIndex(m_compositing->windowThumbnail());
    connect(m_compositing, &Compositing::windowThumbnailChanged, m_form.windowThumbnail, &QComboBox::setCurrentIndex);
    connect(m_form.windowThumbnail, currentIndexChangedSignal, m_compositing, &Compositing::setWindowThumbnail);
    connect(m_form.windowThumbnail, currentIndexChangedSignal,
        [this](int index) {
            if (index == 2) {
                m_form.windowThumbnailWarning->animatedShow();
            } else {
                m_form.windowThumbnailWarning->animatedHide();
            }
        }
    );

    // windows blocking compositing
    m_form.windowsBlockCompositing->setChecked(m_compositing->windowsBlockCompositing());
    connect(m_compositing, &Compositing::windowsBlockCompositingChanged, m_form.windowsBlockCompositing, &QCheckBox::setChecked);
    connect(m_form.windowsBlockCompositing, &QCheckBox::toggled, m_compositing, &Compositing::setWindowsBlockCompositing);

    // compositing type
    CompositingType *type = new CompositingType(this);
    m_form.type->setModel(type);
    auto updateCompositingType = [this, type]() {
        m_form.type->setCurrentIndex(type->indexForCompositingType(m_compositing->compositingType()));
    };
    updateCompositingType();
    connect(m_compositing, &Compositing::compositingTypeChanged,
        [updateCompositingType]() {
            updateCompositingType();
        }
    );
    auto showHideBasedOnType = [this, type]() {
        const int currentType = type->compositingTypeForIndex(m_form.type->currentIndex());
        const bool currentIsOpenGL = currentType == CompositingType::OPENGL31_INDEX ||
                                     currentType == CompositingType::OPENGL20_INDEX;
        auto &layout = m_form.formLayout;

        // Remove all backend-specific rows from the form layout
        for (auto *widget : { m_form.glScaleFilter, m_form.xrScaleFilter, m_form.tearingPrevention, m_form.vulkanDevice, m_form.vulkanVSync }) {
            if (layout->indexOf(widget) != -1) {
                layout->takeRow(widget);
            }
        }

        // Now insert the rows for the selected backend
        constexpr int firstRow = 7;

        if (currentIsOpenGL) {
            layout->insertRow(firstRow + 0, m_form.glScaleFilterLabel,     m_form.glScaleFilter);
            layout->insertRow(firstRow + 1, m_form.tearingPreventionLabel, m_form.tearingPrevention);
        } else if (currentType == CompositingType::XRENDER_INDEX) {
            layout->insertRow(firstRow + 0, m_form.xrScaleFilterLabel,     m_form.xrScaleFilter);
            layout->insertRow(firstRow + 1, m_form.tearingPreventionLabel, m_form.tearingPrevention);
        } else if (currentType == CompositingType::VULKAN_INDEX) {
            layout->insertRow(firstRow + 0, m_form.vulkanDeviceLabel,      m_form.vulkanDevice);
            layout->insertRow(firstRow + 1, m_form.vulkanVSyncLabel,       m_form.vulkanVSync);
        }

        m_form.glScaleFilter->setVisible(currentIsOpenGL);
        m_form.glScaleFilterLabel->setVisible(currentIsOpenGL);
        m_form.xrScaleFilter->setVisible(currentType == CompositingType::XRENDER_INDEX);
        m_form.xrScaleFilterLabel->setVisible(currentType == CompositingType::XRENDER_INDEX);
        m_form.tearingPrevention->setVisible(currentType != CompositingType::VULKAN_INDEX);
        m_form.tearingPreventionLabel->setVisible(currentType != CompositingType::VULKAN_INDEX);
        m_form.vulkanDevice->setVisible(currentType == CompositingType::VULKAN_INDEX);
        m_form.vulkanDeviceLabel->setVisible(currentType == CompositingType::VULKAN_INDEX);
        m_form.vulkanVSync->setVisible(currentType == CompositingType::VULKAN_INDEX);
        m_form.vulkanVSyncLabel->setVisible(currentType == CompositingType::VULKAN_INDEX);
    };
    showHideBasedOnType();
    connect(m_form.type, currentIndexChangedSignal,
        [this, type, showHideBasedOnType]() {
            m_compositing->setCompositingType(type->compositingTypeForIndex(m_form.type->currentIndex()));
            showHideBasedOnType();
        }
    );

    if (m_compositing->OpenGLIsUnsafe()) {
        m_form.glCrashedWarning->animatedShow();
    }
}

void KWinCompositingSettings::load()
{
    KCModule::load();
    m_compositing->reset();
}

void KWinCompositingSettings::defaults()
{
    KCModule::defaults();
    m_compositing->defaults();
}

void KWinCompositingSettings::save()
{
    KCModule::save();
    m_compositing->save();
}

KWinCompositingKCM::KWinCompositingKCM(QWidget* parent, const QVariantList& args, KWin::Compositing::EffectView::ViewType viewType)
    : KCModule(parent, args)
    , m_view(new KWin::Compositing::EffectView(viewType))
{
    QVBoxLayout *vl = new QVBoxLayout(this);

    vl->addWidget(m_view.data());
    setLayout(vl);
    connect(m_view.data(), &KWin::Compositing::EffectView::changed, [this]{
        emit changed(true);
    });
    m_view->setFocusPolicy(Qt::StrongFocus);
}

KWinCompositingKCM::~KWinCompositingKCM()
{
}

void KWinCompositingKCM::save()
{
    m_view->save();
    KCModule::save();
}

void KWinCompositingKCM::load()
{
    m_view->load();
    KCModule::load();
}

void KWinCompositingKCM::defaults()
{
    m_view->defaults();
    KCModule::defaults();
}

K_PLUGIN_FACTORY(KWinCompositingConfigFactory,
                 registerPlugin<KWinDesktopEffects>("effects");
                 registerPlugin<KWinCompositingSettings>("compositing");
                )

#include "main.moc"
