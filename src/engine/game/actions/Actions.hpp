#pragma once
#include "engine/game/registry/Components.hpp"
#include "engine/input/inputmanager.hpp"
#include "engine/utils/containers/octree.hpp"
#include "engine/utils/meshloader.hpp"
#include "engine/graphics/renderer/meshrenderer.hpp"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <set>
#include <string>
#include <vector>

namespace game {
	class Actions {
	public:
		static void Draw(graphics::MeshRenderer &_meshRenderer, Registry &_registry) {
			_registry.execute<Mesh, Texture, Transform, Visibility>([&](Mesh _mesh, Texture _texture, Transform &_transform, Visibility &_visibility) {
				if (_visibility.visible) {
					_meshRenderer.draw(*_mesh.mesh, *_texture.texture, glm::scale(glm::translate(glm::mat4(1), _transform.position) * glm::toMat4(_transform.rotation), _transform.scale));
				}
			});
		}

		static void UpdateRotation(Registry &_registry, float _deltaTime) {
			_registry.execute<Transform, AngularVelocity>([&](Transform &_transform, AngularVelocity &_angularVelocity) {
				_transform.rotation = glm::slerp(_transform.rotation, _transform.rotation * _angularVelocity.angularVelocity, _deltaTime / 5);
			});
		}

		static void UpdateCratePosition(Registry &_registry, float _deltaTime) {
			_registry.execute<Velocity, Transform, ObjectType>([&](Velocity &_velocity, Transform &_transform, ObjectType &_objectType) {
				if (_objectType.type == 0) {
					if (_transform.position.x >= 35 || _transform.position.x <= -35) {
						_velocity.velocity.x = -_velocity.velocity.x;
					}
					if (_transform.position.y >= 25 || _transform.position.y <= -25) {
						_velocity.velocity.y = -_velocity.velocity.y;
					}

					if (_transform.position.z >= 25 || _transform.position.z <= -25) {
						_velocity.velocity.z = -_velocity.velocity.z;
					}
				}
				_transform.position.x += _velocity.velocity.x * _deltaTime;
				_transform.position.y += _velocity.velocity.y * _deltaTime;
				_transform.position.z += _velocity.velocity.z * _deltaTime;
			});
		}

		static void AddAABB(Registry &_registry, Entity &_ent, const std::string &_path, graphics::Camera &_camera, bool _projectile) {
			Transform &transformData = _registry.getComponentUnsafe<Transform>(_ent);
			glm::mat4 transformMatrix = glm::translate(glm::mat4(1), transformData.position) * glm::toMat4(transformData.rotation);
			glm::vec3 min;
			glm::vec3 max;

			glm::vec3 min_trans;
			glm::vec3 max_trans;

			glm::vec3 translationFace;

			auto meshData = utils::MeshLoader::get(_path.c_str());

			min = meshData->positions[0];
			max = meshData->positions[0];

			min_trans = glm::vec3(_camera.getViewProjection() * transformMatrix * glm::vec4(meshData->positions[0], 1));
			max_trans = glm::vec3(_camera.getViewProjection() * transformMatrix * glm::vec4(meshData->positions[0], 1));

			for (auto &face : meshData->positions) {
				translationFace = glm::vec3(_camera.getViewProjection() * transformMatrix * glm::vec4(face, 1));
				if (min.x > face.x) {
					min.x = face.x;
				}
				if (min.y > face.y) {
					min.y = face.y;
				}
				if (min.z > face.z) {
					min.z = face.z;
				}
				if (max.x < face.x) {
					max.x = face.x;
				}
				if (max.y < face.y) {
					max.y = face.y;
				}
				if (max.z < face.z) {
					max.z = face.z;
				}
				if (min_trans.x > translationFace.x) {
					min_trans.x = translationFace.x;
				}
				if (min_trans.y > translationFace.y) {
					min_trans.y = translationFace.y;
				}
				if (min_trans.z > translationFace.z) {
					min_trans.z = translationFace.z;
				}
				if (max_trans.x < translationFace.x) {
					max_trans.x = translationFace.x;
				}
				if (max_trans.y < translationFace.y) {
					max_trans.y = translationFace.y;
				}
				if (max_trans.z < translationFace.z) {
					max_trans.z = translationFace.z;
				}
			}
			_registry.addComponent<AABB>(_ent, math::AABB<3>(min, max), math::AABB<3>(min_trans, max_trans), _projectile);
		}

		static void UpdateAABB(Registry &_registry, graphics::Camera &_camera) {
			_registry.execute<Transform, AABB>([&](Transform &_transform, AABB &_aabb) {
				std::vector<glm::vec3> box;
				box.push_back(glm::vec3(_aabb.untransformed_box.min.x, _aabb.untransformed_box.min.y, _aabb.untransformed_box.min.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.min.x, _aabb.untransformed_box.min.y, _aabb.untransformed_box.max.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.min.x, _aabb.untransformed_box.max.y, _aabb.untransformed_box.min.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.min.x, _aabb.untransformed_box.max.y, _aabb.untransformed_box.max.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.max.x, _aabb.untransformed_box.min.y, _aabb.untransformed_box.min.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.max.x, _aabb.untransformed_box.min.y, _aabb.untransformed_box.max.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.max.x, _aabb.untransformed_box.max.y, _aabb.untransformed_box.min.z));
				box.push_back(glm::vec3(_aabb.untransformed_box.max.x, _aabb.untransformed_box.max.y, _aabb.untransformed_box.max.z));

				glm::mat4 translationMatrix = glm::translate(glm::mat4(1), _transform.position) * glm::toMat4(_transform.rotation);

				glm::vec3 min = glm::vec3(_camera.getViewProjection() * translationMatrix * glm::vec4(box[0], 1));
				glm::vec3 max = glm::vec3(_camera.getViewProjection() * translationMatrix * glm::vec4(box[0], 1));

				glm::vec3 translationFace;

				for (auto &vec : box) {
					translationFace = glm::vec3(_camera.getViewProjection() * translationMatrix * glm::vec4(vec, 1));
					if (min.x > translationFace.x) {
						min.x = translationFace.x;
					}
					if (min.y > translationFace.y) {
						min.y = translationFace.y;
					}
					if (min.z > translationFace.z) {
						min.z = translationFace.z;
					}
					if (max.x < translationFace.x) {
						max.x = translationFace.x;
					}
					if (max.y < translationFace.y) {
						max.y = translationFace.y;
					}
					if (max.z < translationFace.z) {
						max.z = translationFace.z;
					}
				}

				_aabb.transformed_box.min = min;
				_aabb.transformed_box.max = max;
			});
		}
		static int CollisionCheck(Registry &_registry) {
			utils::SparseOctree<Entity, 3, float> collisionTree;
			_registry.execute<Entity, AABB>([&](Entity &_ent, AABB &_aabb) {
				if (!_aabb.projectile) {
					collisionTree.insert(_aabb.transformed_box, _ent);
				}
			});
			std::set<Entity> hitsSet;
			_registry.execute<AABB>([&](AABB &_aabb) {
				if (_aabb.projectile) {
					utils::SparseOctree<Entity, 3, float>::AABBQuery projectileQuery(_aabb.transformed_box);
					collisionTree.traverse(projectileQuery);
					for (size_t i = 0; i < projectileQuery.hits.size(); i++) {
						hitsSet.insert(projectileQuery.hits[i]);
					}
				}
			});
			for (auto element : hitsSet) {
				_registry.erase(element);
			}
			return hitsSet.size();
		}

		static void deleteFarAwayPlanets(Registry &_registry, float &_renderDistance /*actually dependant on cameraposition*/) {
			_registry.execute<Entity, Transform>([&](Entity &_ent, Transform &_transform) {
				if (_transform.position.z <= -_renderDistance) {
					_registry.erase(_ent);
				}
			});
		}

		static void springPhysics(Registry& _registry, float _deltaTime) {
			_registry.execute<Velocity, Transform>([&](Velocity& _velocity, Transform& _transform) {
				_velocity.velocity.x = _velocity.velocity.x - (_transform.position.x * _deltaTime);
				_transform.position += _velocity.velocity * _deltaTime;
				});
		}

		static void cameraMovement(glm::vec3 &_pos, float &_rot, float _deltaTime, graphics::Camera &_camera, glm::vec3 _cameraStartPosition) {
			if (input::InputManager::isKeyPressed(input::Key::A)) {
				_pos.x -= _deltaTime * 5;
			}
			if (input::InputManager::isKeyPressed(input::Key::D)) {
				_pos.x += _deltaTime * 5;
			}
			if (input::InputManager::isKeyPressed(input::Key::W)) {
				_pos.z -= _deltaTime * 5;
			}
			if (input::InputManager::isKeyPressed(input::Key::S)) {
				_pos.z += _deltaTime * 5;
			}
			if (input::InputManager::isKeyPressed(input::Key::Q)) {
				_rot -= _deltaTime;
			}
			if (input::InputManager::isKeyPressed(input::Key::E)) {
				_rot += _deltaTime;
			}
			auto location = _cameraStartPosition + _pos;
			auto rotation = glm::quat(glm::vec3(0, glm::radians(0.0) + _rot * .5, 0));
			_camera.setView(glm::translate(-location) * glm::toMat4(rotation));
		}
	};
}// namespace game