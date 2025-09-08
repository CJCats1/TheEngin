#include "SpriteComponent.h"
#include <DX3D/Graphics/GraphicsEngine.h>

using namespace dx3d;


SpriteComponent::SpriteComponent(GraphicsDevice& device, const std::wstring& texturePath,
	float width, float height) : m_device(device) {
	// Load texture
	m_texture = Texture2D::LoadTexture2D(device.getD3DDevice(), texturePath.c_str());
	if (!m_texture) {
		// Fallback to white texture if loading fails
		m_texture = Texture2D::CreateDebugTexture(device.getD3DDevice());
	}

	initialize(device, width, height);
}

SpriteComponent::SpriteComponent(GraphicsDevice& device, std::shared_ptr<Texture2D> texture,
	float width, float height)
	: m_device(device), m_texture(texture) {
	if (!m_texture) {
		m_texture = Texture2D::CreateDebugTexture(device.getD3DDevice());
	}

	initialize(device, width, height);
}

void SpriteComponent::initialize(GraphicsDevice& device, float width, float height) {
	// Create textured quad mesh
	m_width = width;
	m_height = height;
	m_mesh = Mesh::CreateQuadTextured(device, width, height);
	if (m_mesh && m_texture) {
		m_mesh->setTexture(m_texture);
	}
}

void SpriteComponent::setTexture(std::shared_ptr<Texture2D> texture) {
	m_texture = texture;
	if (m_mesh && m_texture) {
		m_mesh->setTexture(m_texture);
	}
}

void SpriteComponent::draw(DeviceContext& ctx) const {
	if (!isVisible() || !isValid()) return;
	float screenWidth = GraphicsEngine::getWindowWidth();
	float screenHeight = GraphicsEngine::getWindowHeight();
	// Choose alpha/depth states used by your engine
	ctx.enableAlphaBlending();
	ctx.enableTransparentDepth();

	// If sprite is in screen-space, compute a world matrix that places the *center*
	// of the centered quad at the requested pixel coordinates. CreateWorldMatrix2D
	if (m_useScreenSpace) {

		float normalizedX = m_screenPosition.x - 0.5f;  // [0,1] -> [-0.5, 0.5]
		float normalizedY = m_screenPosition.y - 0.5f; // [0,1] -> [-0.5, 0.5]

		float worldX = normalizedX * (screenWidth);// (m_width);   // Scale to world units
		float worldY = normalizedY * (screenHeight); // (m_height);  // Scale to world units

		// Create world matrix for the sprite
		Mat4 worldMatrix =
			Mat4::translation(Vec3(worldX, worldY, 0.0f));

		// Use identity view matrix (no camera transform)
		Mat4 viewMatrix = Mat4::identity();

		// Use the same orthographic projection as the camera
		Mat4 projMatrix = Mat4::orthographic(screenWidth, screenHeight, -100.0f, 100.0f);

		ctx.setWorldMatrix(worldMatrix);
		ctx.setViewMatrix(viewMatrix);
		ctx.setProjectionMatrix(projMatrix);
	}
	else {
		// default: use transform's 2D world matrix
		ctx.setWorldMatrix(getWorldMatrix());
	}

	// Set tint (PS expects it in b1)
	ctx.setTint(m_tint);

	// Bind texture + draw
	m_mesh->draw(ctx);

	// restore states
	ctx.disableAlphaBlending();
	ctx.enableDefaultDepth();
}
